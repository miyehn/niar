#include "ShaderModuleAsset.h"
#include "../Render/Vulkan/Vulkan.hpp"
#include "../Render/Vulkan/VulkanUtils.h"
#include "Utils/myn/Log.h"
#include "Utils/myn/Misc.h"
#include "Utils/myn/ThreadSafeQueue.h"
#include <shaderc/shaderc.hpp>
#include <filesystem>
#include <thread>

#include "Utils/myn/Timer.h"

// ---------------------------------------------------------------------------
// Hard-coded list of every entry-point shader.
// Add a new row here whenever a new shader is introduced.
// ---------------------------------------------------------------------------
ShaderModuleAsset::ShaderModuleDef ShaderModuleAsset::_shaderModuleDefs[] = {
	{ "shaders/geometry.vert",              "main", shaderc_vertex_shader },
	{ "shaders/geometry.frag",              "main", shaderc_fragment_shader },
	{ "shaders/translucency_lit.frag",      "main", shaderc_fragment_shader },
	{ "shaders/simple_gltf.frag",           "main", shaderc_fragment_shader },
	{ "shaders/fullscreen_triangle.vert",   "main", shaderc_vertex_shader },
	{ "shaders/post_processing.frag",       "main", shaderc_fragment_shader },
	{ "shaders/deferred_lighting.frag",     "main", shaderc_fragment_shader },
	{ "shaders/deferred_lighting_shadow.frag", "main", shaderc_fragment_shader },
	{ "shaders/debug_point.vert",           "main", shaderc_vertex_shader },
	{ "shaders/debug_point.frag",           "main", shaderc_fragment_shader },
	{ "shaders/envmap_visualizer.frag",     "main", shaderc_fragment_shader },
	{ "shaders/sine.comp",                  "main", shaderc_compute_shader },
	{ "shaders/sky_transmittance_lut.comp", "main", shaderc_compute_shader },
	{ "shaders/sky_view_lut.comp",          "main", shaderc_compute_shader },
	{ "shaders/ray_gen.rgen",               "main", shaderc_raygen_shader },
	{ "shaders/ray_chit.rchit",             "main", shaderc_closesthit_shader },
	{ "shaders/ray_chit2.rchit",            "main", shaderc_closesthit_shader },
	{ "shaders/ray_miss.rmiss",             "main", shaderc_miss_shader },
	{ "shaders/ray_miss2.rmiss",            "main", shaderc_miss_shader },
};

// ---------------------------------------------------------------------------
// TrackingIncluder: resolves #include paths, reads files, and records dependencies.
// Resolution strategy: relative to the requesting file's directory first,
// then falls back to the shaders/ root.
// Resolved absolute paths are appended to tracked_paths for hot-reload tracking.
// ---------------------------------------------------------------------------
class TrackingIncluder : public shaderc::CompileOptions::IncluderInterface
{
public:
	std::vector<std::string>* tracked_paths = nullptr;  // pointer to ShaderModuleAsset's _dependency_files

	shaderc_include_result* GetInclude(
		const char* requested_source,
		shaderc_include_type /*type*/,
		const char* requesting_source,
		size_t /*include_depth*/) override
	{
		namespace fs = std::filesystem;

		// Try relative to requesting file's directory first.
		fs::path candidate = fs::path(requesting_source).parent_path() / requested_source;
		if (!fs::exists(candidate)) {
			// Fall back to shaders/ root.
			candidate = fs::path(ROOT_DIR) / "shaders" / requested_source;
		}

		auto* result = new shaderc_include_result{};
		if (!fs::exists(candidate)) {
			static const char* kErrorMsg = "file not found";
			result->source_name = "";
			result->source_name_length = 0;
			result->content = kErrorMsg;
			result->content_length = strlen(kErrorMsg);
			return result;
		}

		std::string abs_path = fs::absolute(candidate).string();
		auto content_bytes = myn::read_file(abs_path);

		// Record this dependency for tracking.
		if (tracked_paths) {
			tracked_paths->push_back(abs_path);
		}

		char* name_buf = new char[abs_path.size() + 1];
		memcpy(name_buf, abs_path.c_str(), abs_path.size() + 1);

		char* content_buf = new char[content_bytes.size()];
		memcpy(content_buf, content_bytes.data(), content_bytes.size());

		result->source_name        = name_buf;
		result->source_name_length = abs_path.size();
		result->content            = content_buf;
		result->content_length     = content_bytes.size();
		return result;
	}

	void ReleaseInclude(shaderc_include_result* data) override
	{
		if (data->source_name_length > 0) {
			delete[] data->source_name;
			delete[] data->content;
		}
		delete data;
	}
};

// ---------------------------------------------------------------------------

static bool compile_shader(
	const ShaderModuleAsset::ShaderModuleDef& moduleDef,
	std::vector<uint32_t>& outSpirv,
	std::vector<std::string>& outDeps,
	std::string& outCompileErr)
{
	std::string abs_entry = std::string(ROOT_DIR) + "/" + moduleDef.entry_file;

	auto source_bytes = myn::read_file(abs_entry);
	std::string source(source_bytes.begin(), source_bytes.end());

	// Prepare tracking for this compile.
	std::vector<std::string> new_deps;

	shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);

	auto includer = std::make_unique<TrackingIncluder>();
	includer->tracked_paths = &new_deps;
	options.SetIncluder(std::move(includer));

	for (const auto& def : moduleDef.defines) {
		auto eq = def.find('=');
		if (eq != std::string::npos)
			options.AddMacroDefinition(def.substr(0, eq), def.substr(eq + 1));
		else
			options.AddMacroDefinition(def);
	}

	auto result = compiler.CompileGlslToSpv(
		source, moduleDef.stage, abs_entry.c_str(), moduleDef.entry_function.c_str(), options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
		outCompileErr = result.GetErrorMessage();
		return false;
	}

	// On success, update tracked dependencies.
	outDeps = std::move(new_deps);
	outDeps.push_back(abs_entry);

	outSpirv = std::vector<uint32_t>(result.cbegin(), result.cend());
	return true;
}

ShaderModuleAsset::ShaderModuleAsset(const ShaderModuleDef& def, const std::vector<uint32_t>& initial_spirv, const std::vector<std::string>& dependency_files)
	: Asset(def.entry_file + ":" + def.entry_function, /*reloadable=*/true)
	, _def(def)
	, _dependency_files(dependency_files)
{
	load_action_internal = [this, initial_spirv]() {

		std::vector<uint32_t> spirv = initial_spirv;
		if (get_version() > 0) {
			// this is a reload, aka will need to call compile_shader ourselves instead of relying on what's passed in from constructor
			std::string err;
			if (!compile_shader(_def, spirv, _dependency_files, err)) {
				WARN("Shader compile error in '%s':\n%s", virtual_path.c_str(), err.c_str())
				return;
			}
		}

		// Destroy previous module before creating the new one.
		if (module != VK_NULL_HANDLE) {
			vkDestroyShaderModule(Vulkan::Instance->device, module, nullptr);
			module = VK_NULL_HANDLE;
		}

		VkShaderModuleCreateInfo createInfo = {
			.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = spirv.size() * sizeof(uint32_t),
			.pCode    = spirv.data(),
		};
		EXPECT(vkCreateShaderModule(Vulkan::Instance->device, &createInfo, nullptr, &module), VK_SUCCESS)

		std::string formattedName = "Shader '" + virtual_path + "'";
		NAME_OBJECT(VK_OBJECT_TYPE_SHADER_MODULE, module, formattedName)
	};

	initialize_or_reload_outdated();
}

void ShaderModuleAsset::compile_all()
{
	struct CompiledShader {
		ShaderModuleDef def;
		std::vector<uint32_t> spirv;
		std::vector<std::string> dependency_files;
	};
	auto shaderCount = std::size(_shaderModuleDefs);
	std::vector<CompiledShader> compiledShaders(shaderCount);

	TIMER_BEGIN

	constexpr uint32_t MAX_THREADS = 8;
	constexpr uint32_t SHADERS_PER_TASK = 2;

	// Build a queue of [start, end) index ranges to compile
	myn::ThreadSafeQueue<std::pair<uint32_t, uint32_t>> taskQueue;
	for (uint32_t start = 0; start < shaderCount; start += SHADERS_PER_TASK) {
		taskQueue.enqueue({ start, std::min(start + SHADERS_PER_TASK, (uint32_t)shaderCount) });
	}

	auto worker = [&]() {
		std::pair<uint32_t, uint32_t> range;
		while (taskQueue.dequeue(range)) {
			for (uint32_t i = range.first; i < range.second; i++) {
				std::string err;
				compiledShaders[i].def = _shaderModuleDefs[i];
				if (!compile_shader(_shaderModuleDefs[i], compiledShaders[i].spirv, compiledShaders[i].dependency_files, err)) {
					auto virtualPath = compiledShaders[i].def.entry_file + ":" + compiledShaders[i].def.entry_function;
					WARN("Shader compile error in '%s':\n%s", virtualPath.c_str(), err.c_str())
				}
			}
		}
	};

	uint32_t numTasks = ((uint32_t)shaderCount + SHADERS_PER_TASK - 1) / SHADERS_PER_TASK;
	uint32_t numThreads = std::min(MAX_THREADS, numTasks);
	std::vector<std::thread> threads(numThreads);
	for (auto& t : threads) t = std::thread(worker);
	for (auto& t : threads) t.join();

	TIMER_END(shaderCompileTime)

	for (const auto& [def, spirv, dependency_files] : compiledShaders) {
		new ShaderModuleAsset(def, spirv, dependency_files);
	}
	LOG("Initial shader compilation took %fs", shaderCompileTime)
}


ShaderModuleAsset* ShaderModuleAsset::get(const std::string& virtual_path)
{
	if (auto existing = Asset::find<ShaderModuleAsset>(virtual_path)) return existing;

	ERR("ShaderModuleAsset '%s' not found — was compile_all() called before renderer init?",
		virtual_path.c_str())
	return nullptr;
}

void ShaderModuleAsset::release_resources()
{
	if (module != VK_NULL_HANDLE) {
		vkDestroyShaderModule(Vulkan::Instance->device, module, nullptr);
		module = VK_NULL_HANDLE;
	}
	Asset::release_resources();
}

bool ShaderModuleAsset::is_outdated()
{
	if (get_version() == 0) return true;

	// Check if any dependency files have changed since last load.
	time_t last = get_last_load_time();
	for (const auto& abs_path : _dependency_files) {
		if (last < myn::get_file_last_write_time(abs_path)) return true;
	}
	return false;
}
