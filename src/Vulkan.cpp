#include "Vulkan.hpp"


Vulkan::Vulkan(SDL_Window* window) {

    this->window = window;

    createInstance(window);
    #ifdef DEBUG
    setupDebugMessenger();
    #endif
    createSurface(window);
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPools();
	createVertexBuffer();
	createIndexBuffer();
    createCommandBuffers();
    createSynchronizationObjects();

}

Vulkan::~Vulkan() {

	// TODO: move to elsewhere
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);

	// stay
    for (int i=0; i<MAX_FRAME_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyCommandPool(device, shortLivedCommandsPool, nullptr);
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    #ifdef DEBUG
    DestroyDebugUtilsMessengerEXT(&instance, &debugMessenger, nullptr);
    #endif
    vkDestroyInstance(instance, nullptr);
}

void Vulkan::drawFrame() {

    // inFlightFences: no other access when commands are being submitted for operations on this image.

    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    // check if a prev frame is using this image
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // now mark this image as being used by this frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    VkSubmitInfo submitInfo = {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffers[imageIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores
    };

    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    EXPECT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]), VK_SUCCESS);

    // because it's possible to present to multiple swap chains..
    VkSwapchainKHR swapChains[] = { swapChain };
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,
        .swapchainCount = 1,
        .pSwapchains = swapChains,
        .pImageIndices = &imageIndex,
        .pResults = nullptr // can just use the function return value when there's just one swap chain
    };
    vkQueuePresentKHR(presentQueue, &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAME_IN_FLIGHT;
}

void Vulkan::createBuffer(
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	VkDeviceMemory& bufferMemory
){
	VkBufferCreateInfo bufferInfo {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	EXPECT(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer), VK_SUCCESS);

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	// typeFilter: bit field of suitable memory types.
	auto findMemoryType = [this](uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if (
				// the index of the memoryType that has at least one set bit in common with typeFilter.
				// Namely, the ith memoryType supports the resource (buffer).
				(typeFilter & (1 << i)) &&
				// and also require it to support all the requested properties
				((memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			) {
				return i;
			}
		}
		ERR("failed to find suitable memory type!");
		throw std::runtime_error("");
	};

	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(
			memRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
	};
	// NOTE: vkAllocateMemory is designed for small number of large chunk allocations.
	// in real apps, should make a custom allocator that allocates once and share it among objects of different offsets.
	// this might help: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
	EXPECT(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory), VK_SUCCESS);

	// associate this memory with the buffer
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void Vulkan::copyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = shortLivedCommandsPool,
		.commandBufferCount = 1,
	};
	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	// start recording to this command buffer
	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	VkBufferCopy copyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size
	};
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	vkEndCommandBuffer(commandBuffer);

	// submit this to the queue
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer
	};
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue); // alternatively use a fence, if want to submit a bunch of commands and wait for them all

	// cleanup
	vkFreeCommandBuffers(device, shortLivedCommandsPool, 1, &commandBuffer);
}

void Vulkan::createVertexBuffer() {

	// use normal as color (for now)
	vertices[0].normal = vec3(1, 0, 0);
	vertices[1].normal = vec3(0, 0, 0);
	vertices[2].normal = vec3(0, 1, 0);
	vertices[3].normal = vec3(1, 1, 0);

	VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

	// create the temporary staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	// copy vertex buffer memory over
	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	// now create the actual vertex buffer
	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBuffer,
		vertexBufferMemory);

	// and copy stuff from staging buffer to vertex buffer
	copyBuffer(vertexBuffer, stagingBuffer, bufferSize);
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Vulkan::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(VERTEX_INDEX_TYPE) * indices.size();
	
	// staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	// copy data to staging buffer
	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	// create the actual index buffer
	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indexBuffer,
		indexBufferMemory);
	
	// move stuff from staging buffer and destroy staging buffer
	copyBuffer(indexBuffer, stagingBuffer, bufferSize);
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Vulkan::createInstance(SDL_Window* window) {

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "niar (Vulkan)",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "no engine",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_API_VERSION_1_2
    };

    //---- extensions to use with sdl ----

    // get required extensions count
    uint32_t numSDLRequiredExtensions;
    EXPECT_M(SDL_Vulkan_GetInstanceExtensions(window, &numSDLRequiredExtensions, nullptr), true, "%s", SDL_GetError());
    // get the extensions' names: "VK_KHR_surface", "VK_MVK_macos_surface"
    std::vector<const char*>enabledExtensions(numSDLRequiredExtensions);
    EXPECT_M(
        SDL_Vulkan_GetInstanceExtensions(window, &numSDLRequiredExtensions, enabledExtensions.data()), 
        true, "%s", SDL_GetError());
    #ifdef DEBUG
    enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif

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

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{}; // only used if validationLayersSupported is true
    if (validationLayersSupported) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }
    #endif

    EXPECT(vkCreateInstance(&createInfo, nullptr, &instance), VK_SUCCESS);
}

void Vulkan::createSurface(SDL_Window* window) {
    surface = VK_NULL_HANDLE;
    EXPECT(SDL_Vulkan_CreateSurface(window, instance, &surface), true);
}

void Vulkan::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {
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
}

void Vulkan::setupDebugMessenger() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo; 
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(&instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		WARN("Failed to setup vulkan debug messenger. Continuing without messenger...");
	}
}

Vulkan::QueueFamilyIndices Vulkan::findQueueFamilies(VkPhysicalDevice device) {
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

inline bool Vulkan::checkDeviceExtensionSupport(VkPhysicalDevice device) {

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

inline Vulkan::SwapChainSupportDetails Vulkan::querySwapChainSupport(VkPhysicalDevice device) {
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

inline bool Vulkan::isDeviceSuitable(VkPhysicalDevice device) {
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

void Vulkan::pickPhysicalDevice() {

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

void Vulkan::createLogicalDevice() {
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

	EXPECT(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device), VK_SUCCESS);

	// get queue handles
	vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
}

VkSurfaceFormatKHR Vulkan::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
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

VkPresentModeKHR Vulkan::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	/* present modes (supported by both gpus on my mac):
	VK_PRESENT_MODE_FIFO_KHR (2)
	VK_PRESENT_MODE_IMMEDIATE_KHR (0)
	*/
	for (VkPresentModeKHR availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) return availablePresentMode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Vulkan::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
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

void Vulkan::createSwapChain() {
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0
		&& imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1, // 1 unless for stereoscopic 3D app
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // VK_IMAGE_USAGE_TRANSFER_DST_BIT if copied from other buffers
		.preTransform = swapChainSupport.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, // for blending with other windows
		.presentMode = presentMode,
		.clipped = VK_TRUE, // so pixels covered by other windows are not rendered
		.oldSwapchain = VK_NULL_HANDLE // needed if want to allow resizing window
	};
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
	if (indices.graphicsFamily.value() != indices.presentFamily.value()) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	EXPECT(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain), VK_SUCCESS);

	// retrieve handles to swapchain images
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount); // 3
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void Vulkan::createImageViews() {
	swapChainImageViews.resize(swapChainImages.size());
	for (int i=0; i<swapChainImageViews.size(); i++) {
		VkImageViewCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = swapChainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = swapChainImageFormat,
			.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.baseMipLevel = 0,
			.subresourceRange.levelCount = 1,
			.subresourceRange.baseArrayLayer = 0,
			.subresourceRange.layerCount = 1
		};
		EXPECT(vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]), VK_SUCCESS);
	}
}

inline std::vector<char> Vulkan::readFile(const std::string& filename) {
	// read binary file from the end
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	EXPECT_M(file.is_open(), true, "failed to open file %s", filename.c_str());
	// get file size from position
	size_t fileSize = (size_t) file.tellg();
	std::vector<char> buffer(fileSize);
	// seek to 0
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}

inline VkShaderModule Vulkan::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = code.size(),
		.pCode = reinterpret_cast<const uint32_t*>(code.data())
	};
	VkShaderModule shaderModule;
	EXPECT(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule), VK_SUCCESS);
	return shaderModule;
}

void Vulkan::createRenderPass() {
	// a render pass: load the attachment, r/w operations (by subpasses), then release it?
	// renderpass -> subpass -> attachmentReferences -> attachment
	// a render pass holds ref to an array of color attachments.
	// A subpass then use an array of attachmentRef to selectively get the attachments and use them
	VkAttachmentDescription colorAttachment = {
		.format = swapChainImageFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // layout when attachment is loaded
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};
	VkAttachmentReference colorAttachmentRef = {
		// matches layout(location=X)
		.attachment = 0,
		// "which layout we'd like it to have during this subpass"
		// so, initialLayout -> this -> finalLayout ?
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef // an array, index matches layout (location=X) out vec4 outColor
	};
	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		// wait for the swap chain to finish reading
		// Question: first access scope includes color_attachment_output as well?? (Isn't it just read by the monitor?)
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		// before we can write to it
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	};
	VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorAttachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};
	EXPECT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass), VK_SUCCESS);
	
}

void Vulkan::createGraphicsPipeline() {

	//-------- shader stages --------
	auto vertShaderCode = readFile("spirv/triangle.vert.spv");
	auto fragShaderCode = readFile("spirv/triangle.frag.spv");
	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertShaderModule,
		.pName = "main", // entry point function (should be main for glsl shaders)
		.pSpecializationInfo = nullptr // for specifying the shader's compile-time constants
	};

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragShaderModule,
		.pName = "main", // entry point function (should be main for glsl shaders)
		.pSpecializationInfo = nullptr // for specifying the shader's compile-time constants
	};

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	//-------- fixed-function stages --------

	//---- input ----
	auto bindingDescription = getBindingDescription();
	auto attributeDescriptions = getAttributeDescriptions();
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
		.pVertexAttributeDescriptions = attributeDescriptions.data()
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};
	//---- viewport state ----
	// may result in transformation of image (which portion of the image content should be rendered?)
	VkViewport viewport = {
		.x = 0.0f, .y = 0.0f,
		.width = (float)swapChainExtent.width, .height = (float)swapChainExtent.height,
		.minDepth = 0.0f, .maxDepth = 1.0f
	};
	// no transformation but clips out everything outside
	VkRect2D scissor = {
		.offset = {0, 0},
		.extent = swapChainExtent
	};
	VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	//---- rasterizer ----
	VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE, // don't cull depth outside but clamp to near and far (used in shadowmapping?)
		.rasterizerDiscardEnable = VK_FALSE, // discard everything
		.polygonMode = VK_POLYGON_MODE_FILL,
		.lineWidth = 1.0f,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE, // may be useful for shadowmapping
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f
	};
	// multisampling along edges for anti aliasing
	VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading = 1.0f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};

	// VkPipelineDepthStencilStateCreateInfo skipped for now

	//---- color blending ----

	// this struct is for per framebuffer
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {
		.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT 
			| VK_COLOR_COMPONENT_G_BIT 
			| VK_COLOR_COMPONENT_B_BIT 
			| VK_COLOR_COMPONENT_A_BIT,
		#if 1 // blending off
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		// is it so? also see unity shader lab: https://docs.unity3d.com/Manual/SL-Blend.html
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD
		#else // alpha blending
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD
		#endif
	};

	// this struct for global blend setting
	VkPipelineColorBlendStateCreateInfo colorBlending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		// if set to true, disables color blending specified in colorBlendAttachment and use bitwise blend instead.
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
		.blendConstants[0] = 0.0f,
		.blendConstants[1] = 0.0f,
		.blendConstants[2] = 0.0f,
		.blendConstants[3] = 0.0f
	};

	//---- dynamic state (just a dummy example) ----
	VkDynamicState dynamicStates[] = {
		// VK_DYNAMIC_STATE_VIEWPORT,
		// VK_DYNAMIC_STATE_LINE_WIDTH
	};
	VkPipelineDynamicStateCreateInfo dynamicState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 0,
		.pDynamicStates = dynamicStates
	};

	//---- pipeline layout ----
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 0,
		.pSetLayouts = nullptr,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};
	EXPECT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout), VK_SUCCESS);

	//---- create the fucking pipeline ----
	VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		// shader stages
		.stageCount = 2,
		.pStages = shaderStages,
		// fixed-function stages config
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssemblyInfo,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = nullptr,
		.pColorBlendState = &colorBlending,
		.pDynamicState = nullptr,
		// layout
		.layout = pipelineLayout,
		// render pass
		.renderPass = renderPass,
		.subpass = 0,
		// others
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	EXPECT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline), VK_SUCCESS);

	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
}

void Vulkan::createFramebuffers() {
	swapChainFramebuffers.resize(swapChainImageViews.size());
	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		VkImageView attachments[] = { swapChainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass, // the render pass it needs to be compatible with
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = swapChainExtent.width,
			.height = swapChainExtent.height,
			.layers = 1
		};

		EXPECT(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]), VK_SUCCESS);
	}
}

void Vulkan::createCommandPools() {
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
	VkCommandPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(),
		.flags = 0
	};
	EXPECT(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool), VK_SUCCESS);
	// for one-time commands
	poolInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	EXPECT(vkCreateCommandPool(device, &poolInfo, nullptr, &shortLivedCommandsPool), VK_SUCCESS);
}

void Vulkan::createCommandBuffers()
{
	commandBuffers.resize(swapChainFramebuffers.size());
	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = (uint32_t) commandBuffers.size()
	};
	EXPECT(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()), VK_SUCCESS);

	for (size_t i=0; i<commandBuffers.size(); i++) {

		// begin recording into command buffer:
		VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = 0,
			.pInheritanceInfo = nullptr
		};
		EXPECT(vkBeginCommandBuffer(commandBuffers[i], &beginInfo), VK_SUCCESS);

		// render pass
		VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
		VkRenderPassBeginInfo renderPassInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = renderPass,
			.framebuffer = swapChainFramebuffers[i],
			.renderArea.offset = {0, 0},
			.renderArea.extent = swapChainExtent,
			.pClearValues = &clearColor
		};
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets); // offset, #bindings, (content)
		vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE);
		vkCmdDrawIndexed(commandBuffers[i], indices.size(), 1, 0, 0, 0);
		vkCmdEndRenderPass(commandBuffers[i]);

		EXPECT(vkEndCommandBuffer(commandBuffers[i]), VK_SUCCESS);
	}
}

void Vulkan::createSynchronizationObjects() {

	imageAvailableSemaphores.resize(MAX_FRAME_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAME_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAME_IN_FLIGHT);
	imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};
	VkFenceCreateInfo fenceInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	for (int i=0; i<MAX_FRAME_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
		{
			ERR("failed to create semaphores");
		}
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL Vulkan::debugCallback(
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
VkResult Vulkan::CreateDebugUtilsMessengerEXT(
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
void Vulkan::DestroyDebugUtilsMessengerEXT(
	VkInstance* instance,
	VkDebugUtilsMessengerEXT* debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(*instance, *debugMessenger, pAllocator);
	}
}
