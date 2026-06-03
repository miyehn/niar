#include "ShaderModuleAsset.h"
#include "../Render/Vulkan/Vulkan.hpp"
#include "../Render/Vulkan/VulkanUtils.h"
#include "Utils/myn/Misc.h"

ShaderModuleAsset::ShaderModuleAsset(const std::string& path)
	: Asset(path, false)
{
	load_action_internal = [this, path]() {
		auto code = myn::read_file(path);
		VkShaderModuleCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = code.size(),
			.pCode = reinterpret_cast<const uint32_t*>(code.data())
		};
		EXPECT(vkCreateShaderModule(Vulkan::Instance->device, &createInfo, nullptr, &module), VK_SUCCESS)

		std::string formattedName = "Shader '" + path + "'";
		NAME_OBJECT(VK_OBJECT_TYPE_SHADER_MODULE, module, formattedName)
	};

	initialize_or_reload_outdated();
}

ShaderModuleAsset* ShaderModuleAsset::get(const std::string& path)
{
	auto existing = Asset::find<ShaderModuleAsset>(path);
	if (existing) return existing;

	return new ShaderModuleAsset(path);
}

void ShaderModuleAsset::release_resources()
{
	if (module != VK_NULL_HANDLE) {
		vkDestroyShaderModule(Vulkan::Instance->device, module, nullptr);
		module = VK_NULL_HANDLE;
	}
	Asset::release_resources();
}
