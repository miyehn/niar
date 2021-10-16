#pragma once

#include "Utils/lib.h"
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

namespace gfx
{
	struct PipelineBuilder;
}

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

	bool isFrameInProgress() const { return isFrameStarted; }
	VkCommandBuffer getCurrentCommandBuffer() const
	{
		EXPECT(isFrameStarted, true)
		return commandBuffers[currentFrame]; // as opposed to currentImageIndex, so cmdbuf is not tied to other resource?
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

private:
	uint32_t currentImageIndex;
	bool isFrameStarted = false;

public:

	void waitDeviceIdle() {
		EXPECT(vkDeviceWaitIdle(device), VK_SUCCESS);
	}

	// Below: testbed for learning Vulkan. Will be moved to Renderer later
	// https://vulkan-tutorial.com/Vertex_buffers/Vertex_input_description

	void immediateSubmit(std::function<void(VkCommandBuffer cmdbuf)> &&fn);

	void copyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size);

	VkDevice device;
	VkFormat swapChainImageFormat;
	VkFormat swapChainDepthFormat;
	VkExtent2D swapChainExtent;

	VmaAllocator memoryAllocator;

private:

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

	VmaAllocatedImage depthImage;
	VkImageView depthImageView;

	VkRenderPass swapChainRenderPass;

	VkCommandPool commandPool;
	VkCommandPool shortLivedCommandsPool;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences; // per frame-in-flight
	std::vector<VkFence> imagesInFlight; // per swap chain image
	VkFence immediateSubmitFence;

	#ifdef DEBUG
	const std::vector<const char*> validationLayers = {
		// NOTE: things that this layer reports seems different from the ones on windows?
	#ifdef _WIN32
		"VK_LAYER_KHRONOS_validation"
	#else
		"MoltenVK"
	#endif
	};
	#endif
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	void createInstance(SDL_Window* window);

	void createSurface(SDL_Window* window);

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

	void createDepthImageAndView();

	void createImageViews();

	static inline std::vector<char> readFile(const std::string& filename);

	inline VkShaderModule createShaderModule(const std::vector<char>& code);

	void createSwapChainRenderPass();

	void createFramebuffers();

	void createCommandPools();

	void createCommandBuffers();

	void createMemoryAllocator();

	void createSynchronizationObjects();

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

	PFN_vkCmdBeginDebugUtilsLabelEXT  fn_vkCmdBeginDebugUtilsLabelEXT = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT  fn_vkCmdEndDebugUtilsLabelEXT = nullptr;

	PFN_vkCmdInsertDebugUtilsLabelEXT fn_vkCmdInsertDebugUtilsLabelEXT = nullptr;
	void cmdInsertDebugLabel(VkCommandBuffer &cmdbuf, const std::string &labelName, const myn::Color color = {1, .9f, .5f, 1});

	PFN_vkSetDebugUtilsObjectNameEXT fn_vkSetDebugUtilsObjectNameEXT = nullptr;
	void setObjectName(VkObjectType objectType, uint64_t objectHandle, const std::string &objectName);

};

class ScopedDrawEvent
{
	VkCommandBuffer &cmdbuf;
public:
	ScopedDrawEvent(VkCommandBuffer &cmdbuf, const std::string &name, myn::Color color = {1, 1, 1, 1}) : cmdbuf(cmdbuf)
	{
		VkDebugUtilsLabelEXT markerInfo {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
			.pLabelName = name.c_str(),
			.color = {color.r, color.g, color.b, color.a}
		};
		Vulkan::Instance->fn_vkCmdBeginDebugUtilsLabelEXT(cmdbuf, &markerInfo);
	}

	~ScopedDrawEvent()
	{
		Vulkan::Instance->fn_vkCmdEndDebugUtilsLabelEXT(cmdbuf);
	}
};
#define SCOPED_DRAW_EVENT(CMDBUF, NAME, ...) ScopedDrawEvent __scopedDrawEvent(CMDBUF, NAME, __VA_ARGS__);

#define DEBUG_LABEL(CMDBUF, NAME, ...) Vulkan::Instance->cmdInsertDebugLabel(CMDBUF, NAME, __VA_ARGS__);

// a hack to allow it to be used within vulkan instance creation
#define NAME_OBJECT(VK_OBJECT_TYPE, OBJECT, NAME) (Vulkan::Instance ? Vulkan::Instance : this)->setObjectName(VK_OBJECT_TYPE, (uint64_t)OBJECT, NAME);