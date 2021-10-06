#pragma once

#include "Utils/lib.h"
#include "Utils/myn/Log.h"
#include <vulkan/vulkan.h>
#include <fstream>
#include <set>
#include <optional>
#include <VulkanMemoryAllocator/vk_mem_alloc.h>
#include <functional>
#include "Render/gfx/gfx.h"

#include "Asset/Mesh.h"

/* references:
https://vulkan-tutorial.com/
https://vkguide.dev/
https://github.com/sopyer/Vulkan/blob/562e653fbbd1f7a83ec050676b744dd082b2ebed/main.c
https://gist.github.com/YukiSnowy/dc31f47448ac61dd6aedee18b5d53858
*/

struct Vulkan {

	static Vulkan* Instance;

	Vulkan(SDL_Window* window);

	~Vulkan();

	VkCommandBuffer beginFrame();
	void beginSwapChainRenderPass(VkCommandBuffer cmdbuf, gfx::Pipeline *pipeline);
	void testDraw(VkCommandBuffer cmdbuf, gfx::Pipeline* pipeline);
	void endSwapChainRenderPass(VkCommandBuffer cmdbuf);
	void endFrame();

	bool isFrameInProgress() const { return isFrameStarted; }
	VkCommandBuffer getCurrentCommandBuffer() const {
		EXPECT(isFrameStarted, true)
		return commandBuffers[currentImageIndex];
	}

private:
	uint32_t currentImageIndex;
	bool isFrameStarted = false;

public:

	void waitDeviceIdle() {
		EXPECT(vkDeviceWaitIdle(device), VK_SUCCESS);
	}

	// Below: testbed for learning Vulkan. Will be moved to Renderer later
	// https://vulkan-tutorial.com/Vertex_buffers/Vertex_input_description

	// about how to cut the binding-th array into strides
	VkVertexInputBindingDescription getBindingDescription() {

		VkVertexInputBindingDescription bindingDescription {
			.binding = 0, // binding index. Just one binding if all vertex data is passed as one array
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX, // data is per-vertex (as opposed to per-instance)
		};

		return bindingDescription;
	}

	// about how to interpret each stride
	std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		attributeDescriptions.push_back(VkVertexInputAttributeDescription{
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = (uint32_t)offsetof(Vertex, position)
		});
		attributeDescriptions.push_back(VkVertexInputAttributeDescription{
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = (uint32_t)offsetof(Vertex, normal)
		});
		return attributeDescriptions;
	}

	/*
	https://developer.nvidia.com/vulkan-memory-management
	"Driver developers recommend that you also store multiple buffers,
	like the vertex and index buffer, into a single VkBuffer and
	use offsets in commands like vkCmdBindVertexBuffers."
	*/
#define VERTEX_INDEX_TYPE uint16_t
#define VK_INDEX_TYPE VK_INDEX_TYPE_UINT16
	std::vector<Vertex> vertices = {
		Vertex(vec3(-0.5, -0.5, 0)),
		Vertex(vec3(0.5, -0.5, 0)),
		Vertex(vec3(0.5, 0.5, 0)),
		Vertex(vec3(-0.5, 0.5, 0))
	};
	std::vector<VERTEX_INDEX_TYPE> indices = {
		0, 1, 2, 2, 3, 0
	};

	struct VmaAllocatedBuffer
	{
		VkBuffer buffer;
		VmaAllocation allocation;
	};

	VmaAllocatedBuffer vertexBuffer;
	VmaAllocatedBuffer indexBuffer;

	// per swapchain image
	std::vector<VmaAllocatedBuffer> uniformBuffers;

	VkDescriptorPool descriptorPool;

	std::vector<VkDescriptorSet> descriptorSets;

	struct UniformBufferObject {
		alignas(16) mat4 ModelMatrix;
		alignas(16) mat4 ViewMatrix;
		alignas(16) mat4 ProjectionMatrix;
	};

	void createBufferVma(
		VkDeviceSize size,
		VkBufferUsageFlags bufferUsage,
		VmaMemoryUsage memoryUsage,
		VmaAllocatedBuffer& outVmaAllocatedBuffer);

	void copyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size);

	void createVertexBuffer();
	void createIndexBuffer();

	void createDescriptorPool();
	void createDescriptorSets(const VkDescriptorSetLayout &descriptorSetLayout);
	void createUniformBuffers();
	void updateUniformBuffer(uint32_t currentImage);

	VkDevice device;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	void initSampleInstance(gfx::Pipeline *pipelin);

private:

	const int MAX_FRAME_IN_FLIGHT = 2;
	size_t currentFrame = 0;

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
	VkPhysicalDeviceProperties physicalDeviceProperties;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	std::vector<VkImageView> swapChainImageViews;

#if 0
	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
#endif

	VkCommandPool commandPool;
	VkCommandPool shortLivedCommandsPool;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences; // per frame-in-flight
	std::vector<VkFence> imagesInFlight; // per swap chain image

	VmaAllocator memoryAllocator;

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
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

	void createImageViews();

	static inline std::vector<char> readFile(const std::string& filename);

	inline VkShaderModule createShaderModule(const std::vector<char>& code);

	void createFramebuffers(const VkRenderPass &renderPass);

	void createCommandPools();

	void createCommandBuffers(gfx::Pipeline *pipeline);

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

};
