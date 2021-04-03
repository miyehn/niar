#include "lib.h"
#include "vulkan/vulkan/vulkan.h"

struct Vulkan {

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;

	Vulkan(SDL_Window* window) {
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		LOGF("%u vulkan extensions supported", extensionCount);

		createInstance(window);

		#ifdef DEBUG
		setupDebugMessenger();
		#endif

		pickPhysicalDevice();
	}

	~Vulkan() {
		#ifdef DEBUG
		DestroyDebugUtilsMessengerEXT(&instance, &debugMessenger, nullptr);
		#endif
		vkDestroyInstance(instance, nullptr);
	}

private:

	void createInstance(SDL_Window* window) {

		VkApplicationInfo appInfo = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "niar (Vulkan)",
			.applicationVersion = VK_MAKE_VERSION(0, 1, 0),
			.pEngineName = "no engine",
			.engineVersion = VK_MAKE_VERSION(0, 1, 0),
			.apiVersion = VK_API_VERSION_1_1
		};

		// extensions to use with sdl
		std::vector<const char*>enabledExtensions(2);
		uint32_t tmp_enabledExtensionCount = enabledExtensions.size();
		if (!SDL_Vulkan_GetInstanceExtensions(window, &tmp_enabledExtensionCount, enabledExtensions.data())) {
			ERR(SDL_GetError());
		}
		#ifdef DEBUG
		enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		#endif
		LOGF("%u vulkan extensions enabled", enabledExtensions.size());

		VkInstanceCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &appInfo,
			.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
			.ppEnabledExtensionNames = enabledExtensions.data(),
			.enabledLayerCount = 0 // may be added later
		};

		#ifdef DEBUG // optionally add a validation layer
		const std::vector<const char*> validationLayers = {
			"MoltenVK"
		};
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		bool validationLayersSupported = true;
		for (const char* layerName : validationLayers) {
			bool layerFound = false;
			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}
			if (!layerFound) {
				validationLayersSupported = false;
				WARNF("Validation layer %s requested but not supported. Running without validation..", layerName);
				break;
			}
		}

		if (validationLayersSupported) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		#endif

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			ERR("Failed to create vulkan instance");
		}
	}

	void setupDebugMessenger() {
		VkDebugUtilsMessengerCreateInfoEXT createInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = 
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | 
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = 
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = Vulkan::debugCallback,
			.pUserData = nullptr
		};
		if (CreateDebugUtilsMessengerEXT(&instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			WARN("Failed to setup vulkan debug messenger. Continuing without messenger...");
		}
	}

	void pickPhysicalDevice() {

	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			ERRF("validation layer: %s", pCallbackData->pMessage);
		}
		else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			WARNF("validation layer: %s", pCallbackData->pMessage);
		} else {
			LOGF("validation layer: %s", pCallbackData->pMessage);
		}
		return VK_TRUE;
	}

	// proxy function that looks up the extension first before calling it
	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance* instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(*instance, pCreateInfo, pAllocator, pDebugMessenger);
		} else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	// proxy function for messenger destruction
	void DestroyDebugUtilsMessengerEXT(VkInstance* instance, VkDebugUtilsMessengerEXT* debugMessenger, const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(*instance, *debugMessenger, pAllocator);
		}
	}

};