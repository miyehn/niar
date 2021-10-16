#include "ShaderModule.h"
#include "Vulkan.hpp"
#include "Utils/myn/Misc.h"

std::unordered_map<std::string, ShaderModule *> ShaderModule::pool;

ShaderModule::ShaderModule(const std::string &path)
{
	auto code = myn::read_file(path);
	VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = code.size(),
		.pCode = reinterpret_cast<const uint32_t *>(code.data())
	};
	EXPECT(vkCreateShaderModule(Vulkan::Instance->device, &createInfo, nullptr, &module), VK_SUCCESS)
}

ShaderModule::~ShaderModule()
{
	vkDestroyShaderModule(Vulkan::Instance->device, module, nullptr);
}

ShaderModule *ShaderModule::get(const std::string &path)
{
	auto it = ShaderModule::pool.find(path);
	if (it != ShaderModule::pool.end()) return (*it).second;

	auto new_module = new ShaderModule(path);
	ShaderModule::pool[path] = new_module;
	return new_module;
}

void ShaderModule::cleanup()
{
	for (auto & it : ShaderModule::pool)
	{
		delete it.second;
	}
}
