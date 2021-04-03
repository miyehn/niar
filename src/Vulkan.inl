#include "logging.h"
#include "vulkan/vulkan/vulkan.h"
#include <set>
#include <optional>

struct Vulkan {

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
		std::optional<uint32_t> computeFamily;
		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	SDL_Window* window;

	VkInstance instance;
	VkSurfaceKHR surface;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDevice;
	VkDevice device;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	#ifdef DEBUG
	const std::vector<const char*> validationLayers = {
		"MoltenVK"
	};
	#endif
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	Vulkan(SDL_Window* window) {

		this->window = window;

		createInstance(window);

		#ifdef DEBUG
		setupDebugMessenger();
		#endif

		createSurface(window);

		pickPhysicalDevice();

		createLogicalDevice();

		createSwapChain();
	}

	~Vulkan() {
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyDevice(device, nullptr);
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
			.apiVersion = VK_API_VERSION_1_2
		};

		// extensions to use with sdl
		std::vector<const char*>enabledExtensions(2);
		uint32_t tmp_enabledExtensionCount = enabledExtensions.size();
		if (!SDL_Vulkan_GetInstanceExtensions(window, &tmp_enabledExtensionCount, enabledExtensions.data())) {
			ERR("%s", SDL_GetError());
		}
		#ifdef DEBUG
		enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		#endif
		LOG("%lu vulkan extensions enabled", enabledExtensions.size());

		VkInstanceCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &appInfo,
			.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
			.ppEnabledExtensionNames = enabledExtensions.data(),
			.enabledLayerCount = 0 // may be added later
		};

		#ifdef DEBUG // optionally add a validation layer
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
				WARN("Validation layer %s requested but not supported. Running without validation..", layerName);
				break;
			}
		}

		if (validationLayersSupported) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		#endif

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			ERR("failed to create vulkan instance");
		}
	}

	void createSurface(SDL_Window* window) {
		surface = VK_NULL_HANDLE;
		if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) {
			ERR("failed to create vulkan surface");
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

	inline QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		QueueFamilyIndices indices;

		int i = 0;
		for (const auto &queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport) {
				indices.presentFamily = i;
			}
			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
				indices.computeFamily = i;
			}
			i++;
		}

		return indices;
	}

	inline bool checkDeviceExtensionSupport(VkPhysicalDevice device) {

		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;
		// capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
		// formats
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}
		// present modes
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	inline bool isDeviceSuitable(VkPhysicalDevice device) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(device, &properties);

		#if 0
		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(device, &features);
		#endif

		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

			/*
			chooseSwapSurfaceFormat(swapChainSupport.formats);
			chooseSwapPresentMode(swapChainSupport.presentModes);
			chooseSwapExtent(swapChainSupport.capabilities); 
			*/
		}


		if (properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
			&& queueFamilyIndices.isComplete()
			&& extensionsSupported
			&& swapChainAdequate
		) {
			return true;
		}
		return false;
	}

	void pickPhysicalDevice() {

		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		physicalDevice = VK_NULL_HANDLE;
		for (const auto& device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
			}
		}
		if (physicalDevice == VK_NULL_HANDLE) {
			ERR("failed to find a suitable GPU with vulkan support!");
		}
	}

	void createLogicalDevice() {
		// command queue
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {
			queueFamilyIndices.graphicsFamily.value(),
			queueFamilyIndices.presentFamily.value()
		};
		float queuePriority = 1.0f;
		for (uint32_t queueFamilyIndex : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = queueFamilyIndex,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority
			};
			queueCreateInfos.push_back(queueCreateInfo);
		}

		// features
		VkPhysicalDeviceFeatures deviceFeatures{};

		// logical device create info
		VkDeviceCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pQueueCreateInfos = queueCreateInfos.data(),
			.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
			.pEnabledFeatures = &deviceFeatures,
			.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
			.ppEnabledExtensionNames = deviceExtensions.data()
		};

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			ERR("failed to create logical device");
		}

		// get queue handles
		vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		/* formats: 
		VK_FORMAT_B8G8R8A8_UNORM (44)
		VK_FORMAT_B8G8R8A8_SRGB (50)
		VK_FORMAT_R16G16B16A16_SFLOAT (97)
		VK_FORMAT_A2B10G10R10_UNORM_PACK32 (64)
		VK_FORMAT_A2R10G10B10_UNORM_PACK32 (58)
		color space: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		*/
		for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
			&& availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}
		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		/* present modes (supported by both gpus on my mac):
		VK_PRESENT_MODE_FIFO_KHR (2)
		VK_PRESENT_MODE_IMMEDIATE_KHR (0)
		*/
		for (VkPresentModeKHR availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) return availablePresentMode;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		// on mac, width and height range: [1, 16384]
		if (capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		} else {
			int w, h;
			SDL_Vulkan_GetDrawableSize(window, &w, &h);
			VkExtent2D actualExtent = {
				static_cast<uint32_t>(w),
				static_cast<uint32_t>(h)
			};
			actualExtent.width = std::max(capabilities.minImageExtent.width, 
				std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, 
				std::min(capabilities.maxImageExtent.height, actualExtent.height));
			return actualExtent;
		}
	}

	void createSwapChain() {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			VKERR("%s", pCallbackData->pMessage);
		}
		else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			VKWARN("%s", pCallbackData->pMessage);
		} else {
			VKLOG("%s", pCallbackData->pMessage);
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
	void DestroyDebugUtilsMessengerEXT(
		VkInstance* instance,
		VkDebugUtilsMessengerEXT* debugMessenger,
		const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(*instance, *debugMessenger, pAllocator);
		}
	}

};