#pragma once

#include "Utils/myn/Log.h"
#include <vulkan/vulkan.h>
#include <fstream>
#include <set>
#include <optional>
#include <VulkanMemoryAllocator/vk_mem_alloc.h>
#include <functional>
#include "Buffer.h"
#include "Utils/myn/Color.h"

/* references:
https://vulkan-tutorial.com/
https://vkguide.dev/
https://github.com/sopyer/Vulkan/blob/562e653fbbd1f7a83ec050676b744dd082b2ebed/main.c
https://gist.github.com/YukiSnowy/dc31f47448ac61dd6aedee18b5d53858
*/

struct SDL_Window;

struct VmaAllocatedImage
{
	VkImage image;
	VmaAllocation allocation;
};

struct Vulkan {

	static Vulkan* Instance;

	Vulkan(SDL_Window* window);

	~Vulkan();

	VkCommandBuffer beginFrame();
	void beginSwapChainRenderPass(VkCommandBuffer cmdbuf);
	void endSwapChainRenderPass(VkCommandBuffer cmdbuf);
	void endFrame();

	void immediateSubmit(std::function<void(VkCommandBuffer cmdbuf)> &&fn);

	void initImGui();

	bool isFrameInProgress() const { return isFrameStarted; }

	VkCommandBuffer getCurrentCommandBuffer() const
	{
		EXPECT(isFrameStarted, true)
		return commandBuffers[currentFrame]; // as opposed to currentImageIndex, so cmdbuf is not tied to other resource?
	}

	VkImage getCurrentSwapChainImage() const
	{
		EXPECT(isFrameStarted, true)
		return swapChainImages[currentImageIndex];
	}

	uint32_t getNumSwapChainImages() const
	{
		return swapChainImages.size();
	}

	uint32_t getCurrentFrameIndex() const
	{
		EXPECT(isFrameStarted, true)
		return currentFrame;
	}

	VkRenderPass getSwapChainRenderPass() const { return swapChainRenderPass; }

	void waitDeviceIdle()
	{
		EXPECT(vkDeviceWaitIdle(device), VK_SUCCESS);
	}

	VkDevice device;
	VkFormat swapChainImageFormat;
	VkFormat swapChainDepthFormat;
	VkExtent2D swapChainExtent;

	VkDeviceSize minUniformBufferOffsetAlignment;

	VmaAllocator memoryAllocator;

	std::vector<std::function<void()>> destructionQueue;

private:

	uint32_t currentImageIndex;
	bool isFrameStarted = false;

	const int MAX_FRAME_IN_FLIGHT = 2;
	size_t currentFrame = 0;

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
		std::optional<uint32_t> computeFamily;
		bool isComplete() const {
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
	VkPhysicalDeviceProperties physicalDeviceProperties;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkImageView> swapChainImageViews;

	VkRenderPass swapChainRenderPass;

	VkCommandPool commandPool;
	VkCommandPool shortLivedCommandsPool;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences; // per frame-in-flight
	std::vector<VkFence> imagesInFlight; // per swap chain image
	VkFence immediateSubmitFence;

	VkDescriptorPool imguiPool = VK_NULL_HANDLE;

	#ifdef DEBUG
	const std::vector<const char*> validationLayers = {
		// NOTE: things that this layer reports seems different from the ones on windows?
	#ifdef WINOS
		"VK_LAYER_KHRONOS_validation"
	#else
		"MoltenVK"
	#endif
	};
	#endif
	std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	void createInstance();

	void createSurface();

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	void setupDebugMessenger();

	// isDeviceSuitable helper
	Vulkan::QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	// isDeviceSuitable helper
	inline bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	// isDeviceSuitable helper
	inline SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	inline bool isDeviceSuitable(VkPhysicalDevice device);

	void pickPhysicalDevice();

	void createLogicalDevice();

	// createSwapchain helper
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	// createSwapchain helper
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	// createSwapchain helper
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void createSwapChain();

	void createImageViews();

	static inline std::vector<char> readFile(const std::string& filename);

	inline VkShaderModule createShaderModule(const std::vector<char>& code);

	void createSwapChainRenderPass();

	void createFramebuffers();

	void createCommandPools();

	void createCommandBuffers();

	void createMemoryAllocator();

	void createSynchronizationObjects();

	//======== RTX ========

	void initRayTracing();

	//=====================

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	// proxy function that looks up the extension first before calling it
	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance* instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	// proxy function for messenger destruction
	void DestroyDebugUtilsMessengerEXT(
		VkInstance* instance,
		VkDebugUtilsMessengerEXT* debugMessenger,
		const VkAllocationCallbacks* pAllocator);

	void findProxyFunctionPointers();

public:

#define FN_PTR(FN) PFN_##FN fn_##FN = nullptr;

	FN_PTR(vkCmdBeginDebugUtilsLabelEXT)
	FN_PTR(vkCmdEndDebugUtilsLabelEXT)

	FN_PTR(vkCmdInsertDebugUtilsLabelEXT)
	void cmdInsertDebugLabel(VkCommandBuffer &cmdbuf, const std::string &labelName, const myn::Color color = {1, .9f, .5f, 1});

	FN_PTR(vkSetDebugUtilsObjectNameEXT)
	void setObjectName(VkObjectType objectType, uint64_t objectHandle, const std::string &objectName);

	FN_PTR(vkGetAccelerationStructureBuildSizesKHR)
	FN_PTR(vkCreateAccelerationStructureKHR)
	FN_PTR(vkDestroyAccelerationStructureKHR)
	FN_PTR(vkGetAccelerationStructureDeviceAddressKHR)
	FN_PTR(vkCmdBuildAccelerationStructuresKHR)
	FN_PTR(vkCmdWriteAccelerationStructuresPropertiesKHR)
	FN_PTR(vkCmdCopyAccelerationStructureKHR)

};