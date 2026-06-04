#pragma once
#include "Assets/Asset.h"
#include <shaderc/shaderc.h>
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

class ShaderModuleAsset : public Asset
{
public:
	static void compile_all();
	// Pool-only lookup — all shaders must be pre-compiled via compile_all().
	static ShaderModuleAsset* get(const std::string& virtual_path);

	VkShaderModule module = VK_NULL_HANDLE;

	bool is_outdated() override;

protected:
	void release_resources() override;

private:
	struct ShaderModuleDef {
		std::string entry_file;              // e.g. "shaders/geometry.vert"
		std::string entry_function = "main";
		shaderc_shader_kind stage;
		std::vector<std::string> defines;    // e.g. {"USE_NORMAL_MAP=1"}
	};
	explicit ShaderModuleAsset(const ShaderModuleDef& def);

	ShaderModuleDef _def;
	std::vector<std::string> _dependency_files;  // absolute paths of all #included files

	static ShaderModuleDef _shaderModuleDefs[];
	static ShaderModuleAsset* get(const ShaderModuleDef& def);
};
