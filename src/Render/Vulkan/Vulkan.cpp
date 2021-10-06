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
	createMemoryAllocator();
	createCommandPools();
	createSynchronizationObjects();
}

void Vulkan::initSampleInstance(gfx::Pipeline *pipeline)
{
	createFramebuffers(pipeline->getRenderPass());
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets(pipeline->getDescriptorSetLayout());
	createCommandBuffers(pipeline);
}

Vulkan::~Vulkan() {

	// TODO: move to elsewhere
	vmaDestroyBuffer(memoryAllocator, vertexBuffer.buffer, vertexBuffer.allocation);
	vmaDestroyBuffer(memoryAllocator, indexBuffer.buffer, indexBuffer.allocation);
	for (size_t i=0; i<swapChainImages.size(); i++) {
		vmaDestroyBuffer(memoryAllocator, uniformBuffers[i].buffer, uniformBuffers[i].allocation);
	}
	vmaDestroyAllocator(memoryAllocator);

	vkDestroyDescriptorPool(device, descriptorPool, nullptr);

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

VkCommandBuffer Vulkan::beginFrame()
{
	EXPECT(isFrameStarted, false)
	isFrameStarted = true;

	// inFlightFences: no other access when commands are being submitted for operations on this image.

	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &currentImageIndex);
	auto cmdbuf = getCurrentCommandBuffer();

	// check if a prev frame is using this image
	if (imagesInFlight[currentImageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(device, 1, &imagesInFlight[currentImageIndex], VK_TRUE, UINT64_MAX);
	}
	// now mark this image as being used by this frame
	imagesInFlight[currentImageIndex] = inFlightFences[currentFrame];

	// update uniforms
	updateUniformBuffer(currentImageIndex);

	// record command buffer

	// begin recording into command buffer:
	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = 0,
		.pInheritanceInfo = nullptr
	};
	EXPECT(vkBeginCommandBuffer(cmdbuf, &beginInfo), VK_SUCCESS)
	return cmdbuf;
}

void Vulkan::testDraw(VkCommandBuffer cmdbuf, gfx::Pipeline* pipeline)
{
	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipeline());
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmdbuf, 0, 1, &vertexBuffer.buffer, offsets); // offset, #bindings, (content)
	vkCmdBindIndexBuffer(cmdbuf, indexBuffer.buffer, 0, VK_INDEX_TYPE);
	vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipelineLayout(),
							0, // firstSet : uint32_t
							1, // descriptorSetCount : uint32_t
							&descriptorSets[currentImageIndex],
							0, nullptr); // for dynamic descriptors (not reached yet)
	vkCmdDrawIndexed(cmdbuf, indices.size(), 1, 0, 0, 0);
}

void Vulkan::endFrame()
{
	EXPECT(isFrameStarted, true)

	auto cmdbuf = getCurrentCommandBuffer();
	EXPECT(vkEndCommandBuffer(cmdbuf), VK_SUCCESS)

	// submit to queue
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffers[currentImageIndex],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signalSemaphores
	};

	vkResetFences(device, 1, &inFlightFences[currentFrame]);
	EXPECT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]), VK_SUCCESS)

	// because it's possible to present to multiple swap chains..
	VkSwapchainKHR swapChains[] = { swapChain };
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphores,
		.swapchainCount = 1,
		.pSwapchains = swapChains,
		.pImageIndices = &currentImageIndex,
		.pResults = nullptr // can just use the function return value when there's just one swap chain
	};
	vkQueuePresentKHR(presentQueue, &presentInfo);

	currentFrame = (currentFrame + 1) % MAX_FRAME_IN_FLIGHT;
	isFrameStarted = false;
}

void Vulkan::beginSwapChainRenderPass(VkCommandBuffer cmdbuf, gfx::Pipeline *pipeline)
{
	EXPECT(isFrameStarted, true)
	EXPECT(cmdbuf, getCurrentCommandBuffer())

	// render pass
	VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
	VkRect2D renderArea = { .offset = {0, 0}, .extent = swapChainExtent };
	VkRenderPassBeginInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = pipeline->getRenderPass(),
		.framebuffer = swapChainFramebuffers[currentImageIndex],
		.renderArea = renderArea,
		.clearValueCount = 1,
		.pClearValues = &clearColor
	};
	vkCmdBeginRenderPass(cmdbuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Vulkan::endSwapChainRenderPass(VkCommandBuffer cmdbuf)
{
	EXPECT(isFrameStarted, true)
	EXPECT(cmdbuf, getCurrentCommandBuffer())
	vkCmdEndRenderPass(cmdbuf);
}

void Vulkan::createMemoryAllocator()
{
	VmaAllocatorCreateInfo allocatorInfo = {
		.physicalDevice = physicalDevice,
		.device = device,
		.instance = instance
	};
	EXPECT(vmaCreateAllocator(&allocatorInfo, &memoryAllocator), VK_SUCCESS);
}

void Vulkan::createBufferVma(
	VkDeviceSize size,
	VkBufferUsageFlags bufferUsage,
	VmaMemoryUsage memoryUsage,
	VmaAllocatedBuffer& outVmaAllocatedBuffer)
{
	VkBufferCreateInfo bufferCreateInfo {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = bufferUsage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	VmaAllocationCreateInfo vmaAllocCreateInfo = {
		.usage = memoryUsage
	};
	EXPECT(vmaCreateBuffer(
		memoryAllocator,
		&bufferCreateInfo,
		&vmaAllocCreateInfo,
		&outVmaAllocatedBuffer.buffer,
		&outVmaAllocatedBuffer.allocation,
		nullptr),VK_SUCCESS);
}

void Vulkan::copyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = shortLivedCommandsPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
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
	EXPECT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE), VK_SUCCESS)
	EXPECT(vkQueueWaitIdle(graphicsQueue), VK_SUCCESS) // alternatively use a fence, if want to submit a bunch of commands and wait for them all

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

	// create a staging buffer
	VmaAllocatedBuffer stagingBuffer;
	createBufferVma(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

	// copy vertex buffer memory over to staging buffer
	void* mappedMemory;
	vmaMapMemory(memoryAllocator, stagingBuffer.allocation, &mappedMemory);
	memcpy(mappedMemory, vertices.data(), (size_t)bufferSize);
	vmaUnmapMemory(memoryAllocator, stagingBuffer.allocation);

	// now create the actual vertex buffer
	createBufferVma(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, vertexBuffer);

	// and copy stuff from staging buffer to vertex buffer
	copyBuffer(vertexBuffer.buffer, stagingBuffer.buffer, bufferSize);

	// then destroy the staging buffer
	vmaDestroyBuffer(memoryAllocator, stagingBuffer.buffer, stagingBuffer.allocation);
}

void Vulkan::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(VERTEX_INDEX_TYPE) * indices.size();

	// staging buffer
	VmaAllocatedBuffer stagingBuffer;
	createBufferVma(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

	// copy data to staging buffer
	void* mappedMemory;
	vmaMapMemory(memoryAllocator, stagingBuffer.allocation, &mappedMemory);
	memcpy(mappedMemory, indices.data(), (size_t)bufferSize);
	vmaUnmapMemory(memoryAllocator, stagingBuffer.allocation);

	// create the actual index buffer
	auto vkUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	createBufferVma(bufferSize, vkUsage, VMA_MEMORY_USAGE_GPU_ONLY, indexBuffer);

	// move stuff from staging buffer and destroy staging buffer
	copyBuffer(indexBuffer.buffer, stagingBuffer.buffer, bufferSize);
	vmaDestroyBuffer(memoryAllocator, stagingBuffer.buffer, stagingBuffer.allocation);
}

void Vulkan::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	uniformBuffers.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		createBufferVma(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, uniformBuffers[i]);
	}
}

// "want a pool that can hold X uniform buffers, Y images, etc."
// might help: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkDescriptorPoolCreateInfo.html
void Vulkan::createDescriptorPool() {
	VkDescriptorPoolSize poolSize = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = static_cast<uint32_t>(swapChainImages.size())
	};
	VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = static_cast<uint32_t>(swapChainImages.size()),
		.poolSizeCount = 1,
		.pPoolSizes = &poolSize,
	};
	EXPECT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool), VK_SUCCESS)
}

void Vulkan::createDescriptorSets(const VkDescriptorSetLayout &descriptorSetLayout)
{
	std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size()),
		.pSetLayouts = layouts.data(),
	};
	descriptorSets.resize(swapChainImages.size());
	EXPECT(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()), VK_SUCCESS)

	for (size_t i=0; i<swapChainImages.size(); i++)
	{
		VkDescriptorBufferInfo bufferInfo = {
			.buffer = uniformBuffers[i].buffer,
			.offset = 0,
			.range = VK_WHOLE_SIZE,
		};
		// "Structure specifying the parameters of a descriptor set write operation"
		VkWriteDescriptorSet descriptorWrite = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			// actual write data (one of three)
			.pImageInfo = nullptr,
			.pBufferInfo = &bufferInfo, // where in which buffer
			.pTexelBufferView = nullptr,
		};
		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
	}
}

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
void Vulkan::updateUniformBuffer(uint32_t currentImage) {
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	UniformBufferObject ubo = {
		.ModelMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		.ViewMatrix = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		.ProjectionMatrix = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 10.0f),
	};
	// so it's not upside down
	ubo.ProjectionMatrix[1][1] *= -1;

	// upload data
	void* uniformBuffer;
	vmaMapMemory(memoryAllocator, uniformBuffers[currentImage].allocation, &uniformBuffer);
	memcpy(uniformBuffer, &ubo, sizeof(UniformBufferObject));
	vmaUnmapMemory(memoryAllocator, uniformBuffers[currentImage].allocation);
}

void Vulkan::createInstance(SDL_Window* in_window) {

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
    EXPECT_M(SDL_Vulkan_GetInstanceExtensions(in_window, &numSDLRequiredExtensions, nullptr), SDL_TRUE, "%s", SDL_GetError())
    // get the extensions' names: "VK_KHR_surface", "VK_MVK_macos_surface"
    std::vector<const char*>enabledExtensions(numSDLRequiredExtensions);
    EXPECT_M(
        SDL_Vulkan_GetInstanceExtensions(in_window, &numSDLRequiredExtensions, enabledExtensions.data()),
        SDL_TRUE, "%s", SDL_GetError())
    #ifdef DEBUG
    enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
		.enabledLayerCount = 0, // may be added later
		.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size()),
        .ppEnabledExtensionNames = enabledExtensions.data(),
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
            WARN("Validation layer %s requested but not supported. Running without validation..", layerName)
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

    EXPECT(vkCreateInstance(&createInfo, nullptr, &instance), VK_SUCCESS)
}

void Vulkan::createSurface(SDL_Window* in_window) {
    surface = VK_NULL_HANDLE;
    EXPECT(SDL_Vulkan_CreateSurface(in_window, instance, &surface), SDL_TRUE)
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
		WARN("Failed to setup vulkan debug messenger. Continuing without messenger...")
	}
}

Vulkan::QueueFamilyIndices Vulkan::findQueueFamilies(VkPhysicalDevice in_device)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(in_device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(in_device, &queueFamilyCount, queueFamilies.data());

	QueueFamilyIndices queueFamilyIndices;

	int i = 0;
	for (const auto &queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queueFamilyIndices.graphicsFamily = i;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(in_device, i, surface, &presentSupport);
		if (presentSupport) {
			queueFamilyIndices.presentFamily = i;
		}
		if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
			queueFamilyIndices.computeFamily = i;
		}
		i++;
	}

	return queueFamilyIndices;
}

inline bool Vulkan::checkDeviceExtensionSupport(VkPhysicalDevice in_device) {

	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(in_device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(in_device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

inline Vulkan::SwapChainSupportDetails Vulkan::querySwapChainSupport(VkPhysicalDevice in_device)
{
	SwapChainSupportDetails details;
	// capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(in_device, surface, &details.capabilities);
	// formats
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(in_device, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(in_device, surface, &formatCount, details.formats.data());
	}
	// present modes
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(in_device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(in_device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

inline bool Vulkan::isDeviceSuitable(VkPhysicalDevice in_device) {
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(in_device, &properties);

	#if 0
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);
	#endif

	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(in_device);

	bool extensionsSupported = checkDeviceExtensionSupport(in_device);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(in_device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	if (properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU//VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
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
	for (const auto& dvc : devices)
	{
		if (isDeviceSuitable(dvc))
		{
			physicalDevice = dvc;
		}
	}
	if (physicalDevice == VK_NULL_HANDLE)
	{
		ERR("failed to find a suitable GPU with vulkan support!")
	}

	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	VKLOG("Picked physical device \"%s\" which has minimum buffer alignment of %llu bytes",
		physicalDeviceProperties.deviceName, physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
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
		.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
		.ppEnabledExtensionNames = deviceExtensions.data(),
		.pEnabledFeatures = &deviceFeatures,
	};

	EXPECT(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device), VK_SUCCESS)

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

	EXPECT(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain), VK_SUCCESS)

	// retrieve handles to swapchain images
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount); // 3
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void Vulkan::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());
	VkComponentMapping componentMapping = {
		.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.a = VK_COMPONENT_SWIZZLE_IDENTITY
	};
	VkImageSubresourceRange subresourceRange = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1
	};
	for (int i=0; i<swapChainImageViews.size(); i++)
	{
		VkImageViewCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = swapChainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = swapChainImageFormat,
			.components = componentMapping,
			.subresourceRange = subresourceRange,
		};
		EXPECT(vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]), VK_SUCCESS)
	}
}

inline std::vector<char> Vulkan::readFile(const std::string& filename) {
	// read binary file from the end
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	EXPECT_M(file.is_open(), true, "failed to open file %s", filename.c_str())
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
	EXPECT(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule), VK_SUCCESS)
	return shaderModule;
}

void Vulkan::createFramebuffers(const VkRenderPass &renderPass) {
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

		EXPECT(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]), VK_SUCCESS)
	}
}

void Vulkan::createCommandPools() {
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
	VkCommandPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(),
	};
	EXPECT(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool), VK_SUCCESS)
	// for one-time commands
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	EXPECT(vkCreateCommandPool(device, &poolInfo, nullptr, &shortLivedCommandsPool), VK_SUCCESS)
}

void Vulkan::createCommandBuffers(gfx::Pipeline *pipeline)
{
	commandBuffers.resize(swapChainFramebuffers.size());
	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = (uint32_t) commandBuffers.size()
	};
	EXPECT(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()), VK_SUCCESS)
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
			ERR("failed to create semaphores")
		}
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL Vulkan::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		VKERR("%s", pCallbackData->pMessage)
	}
	else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		VKWARN("%s", pCallbackData->pMessage)
	}
#ifdef MYN_VK_VERBOSE
	else
	{
		VKLOG("%s", pCallbackData->pMessage);
	}
#endif
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
