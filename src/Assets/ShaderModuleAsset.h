#pragma once
#include "Assets/Asset.h"
#include <shaderc/shaderc.h>
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

class ShaderModuleAsset : public Asset
{
public:
	struct ShaderModuleDef {
		std::string entry_file;              // e.g. "shaders/geometry.vert"
		std::string entry_function = "main";
		shaderc_shader_kind stage;
		std::vector<std::string> defines;    // e.g. {"USE_NORMAL_MAP=1"}
	};

	static void compile_all();
	// Pool-only lookup — all shaders must be pre-compiled via compile_all().
	static ShaderModuleAsset* get(const std::string& virtual_path);

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
