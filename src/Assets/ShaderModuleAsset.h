#pragma once
#include "Assets/Asset.h"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

enum ShaderStage
{
	SS_Unknown = 0,
	SS_Vertex,
	SS_Fragment,
	SS_Compute,
	SS_RayGen,
	SS_AnyHit,
	SS_ClosestHit,
	SS_Miss,
};

struct ShaderModuleDef
{
	ShaderModuleDef() = default;
	ShaderModuleDef(
		std::string entry_file,
		std::string entry_function = "main",
		ShaderStage stage = SS_Vertex,
		std::vector<std::string> defines = {})
		: entry_file(std::move(entry_file))
		, entry_function(std::move(entry_function))
		, stage(stage)
		, defines(std::move(defines))
	{
	}

	ShaderModuleDef& operator=(std::string newEntryFile)
	{
		entry_file = std::move(newEntryFile);
		entry_function = "main";
		stage = SS_Unknown;
		defines.clear();
		return *this;
	}

	std::string entry_file;              // e.g. "shaders/geometry.vert"
	std::string entry_function = "main";
	ShaderStage stage = SS_Unknown;
	std::vector<std::string> defines;    // e.g. {"USE_NORMAL_MAP=1"}
};

class ShaderModuleAsset : public Asset
{
public:

	static void compile_all();

	// Pool-only lookups - all shaders must be pre-compiled via compile_all().
	static ShaderModuleAsset* get(const std::string& virtual_path);
	static ShaderModuleAsset* get(const ShaderModuleDef& def);
	/*
	static ShaderModuleAsset* get(
		const std::string& entry_file,
		const std::string& entry_function,
		const std::vector<std::string>& defines);
	*/

	VkShaderModule module = VK_NULL_HANDLE;

	bool is_outdated() override;

protected:
	void release_resources() override;

private:
	explicit ShaderModuleAsset(const ShaderModuleDef& def, const std::vector<uint32_t>& initial_spirv, const std::vector<std::string>& dependency_files);

	ShaderModuleDef _def;
	std::vector<std::string> _dependency_files;  // absolute paths of all #included files

	static ShaderModuleDef _shaderModuleDefs[];
};
