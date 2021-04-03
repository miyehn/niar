#include "lib.h"
#include "vulkan/vulkan/vulkan.h"

namespace vulkan {

void init() {
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	LOGF("%u vulkan extensions supported", extensionCount);
}

VkInstance createInstance(SDL_Window* window) {

	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "niar (Vulkan)",
		.applicationVersion = VK_MAKE_VERSION(0, 1, 0),
		.pEngineName = "no engine",
		.engineVersion = VK_MAKE_VERSION(0, 1, 0),
		.apiVersion = VK_API_VERSION_1_2
	};

	uint32_t enabledExtensionCount = 2;
	const char* enabledExtensions[2];
	if (!SDL_Vulkan_GetInstanceExtensions(window, &enabledExtensionCount, enabledExtensions)) {
		ERR(SDL_GetError());
	}
	LOGF("%u vulkan extensions required", enabledExtensionCount);

	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = enabledExtensionCount,
		.ppEnabledExtensionNames = enabledExtensions,
		.enabledLayerCount = 0
	};

	VkInstance instance;
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		ERR("Failed to create vulkan instance");
	}

	return instance;
}

} // namespace vulkan