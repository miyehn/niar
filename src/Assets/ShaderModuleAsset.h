#pragma once
#include "Assets/Asset.h"
#include <vulkan/vulkan.h>
#include <string>

class ShaderModuleAsset : public Asset
{
public:
	static ShaderModuleAsset* get(const std::string& path);

	VkShaderModule module = VK_NULL_HANDLE;

	void release_resources() override;

private:
	explicit ShaderModuleAsset(const std::string& path);
};
