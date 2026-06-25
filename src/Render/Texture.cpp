#include "Texture.h"
#include <stb_image.h>
#include "Render/Vulkan/SamplerCache.h"
#include "Render/Vulkan/VulkanUtils.h"

std::unordered_map<std::string, Texture *> Texture::texturePool;

namespace
{
bool defaultTexturesCreated = false;
}

Texture::~Texture()
{
	for (auto it = texturePool.begin(); it != texturePool.end();)
	{
		if (it->second == this)
			it = texturePool.erase(it);
		else
			++it;
	}
	vmaDestroyImage(Vulkan::Instance->memoryAllocator, resource.image, resource.allocation);
}

//--------

void createTexture2DFromPixelData(
	uint8_t *pixels,
	uint32_t width,
	uint32_t height,
	VkFormat imageFormat,
	uint32_t pixelSize,
	bool generateMips,
	VmaAllocatedImage &outResource,
	VkImageView &outImageView)
{
	VkExtent3D imageExtent = {
		.width = static_cast<uint32_t>(width),
		.height = static_cast<uint32_t>(height),
		.depth = 1
	};

	uint32_t numMips = generateMips ? static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1 : 1;

	VkImageCreateInfo imgInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = imageFormat,
		.extent = imageExtent,
		.mipLevels = numMips,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
	};

	if (generateMips) imgInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationCreateInfo imgAllocInfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY
	};

	vmaCreateImage(Vulkan::Instance->memoryAllocator, &imgInfo, &imgAllocInfo, &outResource.image, &outResource.allocation, nullptr);

	vk::uploadPixelsToImage(pixels, 0, 0, width, height, pixelSize, outResource); // to mip 0
	if (generateMips)
	{
		vk::generateMips(outResource, width, height);
	}

	// create image view
	VkImageViewCreateInfo viewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.flags = 0,
		.image = outResource.image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = imageFormat,
		.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = numMips,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	EXPECT(vkCreateImageView(Vulkan::Instance->device, &viewInfo, nullptr, &outImageView), VK_SUCCESS)
}

#include "TextureFormatMappings.inl"

Texture2D::Texture2D(
	const std::string &name,
	const std::string &path,
	ImageFormat textureFormat,
	const BindlessTexture2DInfo& bindlessInfo)
{
#ifdef DEBUG
	auto it = texturePool.find(path);
	if (it != texturePool.end()) WARN("trying to load texture '%s' that's already in the pool. Overriding..", path.c_str())
	ASSERT(textureFormat.channelDepth % 8 == 0)
#endif

	int native_channels = 0;
	int iwidth = 0, iheight = 0;
	uint8_t* pixels = nullptr;
	if (textureFormat.channelDepth==8) {
		pixels = stbi_load(path.c_str(), &iwidth, &iheight, &native_channels, textureFormat.numChannels);
	} else if (textureFormat.channelDepth==16) {
		pixels = (uint8_t*)stbi_load_16(path.c_str(), &iwidth, &iheight, &native_channels, textureFormat.numChannels);
	} else if (textureFormat.channelDepth==32) {
		pixels = (uint8_t*)stbi_loadf(path.c_str(), &iwidth, &iheight, &native_channels, textureFormat.numChannels);
	} else {
		ERR("Trying to load image '%s' with wrong channelDepth", path.c_str())
	}

	ASSERT(pixels != nullptr)

	imageFormat = getFormatFromMap(textureFormat);

	width = iwidth;
	height = iheight;

	uint32_t pixelSize = textureFormat.numChannels * (textureFormat.channelDepth / 8);
	createTexture2DFromPixelData(pixels, width, height, imageFormat, pixelSize, true, resource, imageView);

	NAME_OBJECT(VK_OBJECT_TYPE_IMAGE, resource.image, name)
	NAME_OBJECT(VK_OBJECT_TYPE_IMAGE_VIEW, imageView, name + "_defaultView")

	stbi_image_free(pixels);
	registerBindless(bindlessInfo);

	//texturePool()[name] = this;
}

void Texture2D::createDefaultTextures()
{
	if (defaultTexturesCreated)
	{
		WARN("Trying to re-create default textures. Skipping..")
		return;
	}
	defaultTexturesCreated = true;

	ASSERT(BindlessResources::Instance != nullptr)

	const VkSamplerCreateInfo defaultSamplerInfo = SamplerCache::defaultInfo();
	const BindlessTexture2DInfo defaultBindlessInfo = {
		.registerTexture = true,
		.samplerInfo = defaultSamplerInfo,
	};

	uint8_t whitePixel[] = {255, 255, 255, 255};
	auto* whiteTexture = new Texture2D(
		"_white",
		whitePixel,
		1,
		1,
		{4, 8, 0},
		false,
		defaultBindlessInfo);

	uint8_t blackPixel[] = {0, 0, 0, 0};
	auto* blackTexture = new Texture2D(
		"_black",
		blackPixel,
		1,
		1,
		{4, 8, 0},
		false,
		defaultBindlessInfo);

	uint8_t defaultNormalPixel[] = {127, 127, 255, 0};
	auto* defaultNormal = new Texture2D(
		"_defaultNormal",
		defaultNormalPixel,
		1,
		1,
		{4, 8, 0},
		false,
		defaultBindlessInfo);

	BindlessResources::Instance->setTexture2DFiller(
		blackTexture->imageView,
		defaultSamplerInfo);

#if TMP_BINDLESS_DEBUG
	BindlessResources::Instance->runDebugSelfTest(
		whiteTexture->bindlessHandle,
		blackTexture->bindlessHandle,
		blackTexture->imageView,
		defaultSamplerInfo);

	runBindlessLifetimeSelfTest();
#endif
}

void Texture2D::cleanupDefaultTextures()
{
	ASSERT(defaultTexturesCreated)

	const char* defaultTextureNames[] = {
		"_white",
		"_defaultNormal",
		"_black",
	};
	for (const char* name : defaultTextureNames)
	{
		auto it = texturePool.find(name);
		ASSERT(it != texturePool.end())

		auto* texture = dynamic_cast<Texture2D*>(it->second);
		ASSERT(texture != nullptr)
		delete texture;
	}
	defaultTexturesCreated = false;
}

#if TMP_BINDLESS_DEBUG
// note [myn]: this function is not reviewed
void Texture2D::runBindlessLifetimeSelfTest()
{
	constexpr const char* unregisteredName =
		"_bindlessLifetimeSelfTestUnregistered";
	constexpr const char* registeredName =
		"_bindlessLifetimeSelfTestRegistered";
	uint8_t pixel[] = {0, 0, 0, 0};

	auto* unregisteredTexture = new Texture2D(
		unregisteredName,
		pixel,
		1,
		1,
		{4, 8, 0},
		false);
	ASSERT(
		unregisteredTexture->getBindlessHandle().index ==
		INVALID_BINDLESS_INDEX)
	delete unregisteredTexture;
	ASSERT(texturePool.find(unregisteredName) == texturePool.end())

	const BindlessTexture2DInfo bindlessInfo = {
		.registerTexture = true,
		.samplerInfo = SamplerCache::defaultInfo(),
	};
	auto* firstRegisteredTexture = new Texture2D(
		registeredName,
		pixel,
		1,
		1,
		{4, 8, 0},
		false,
		bindlessInfo);
	const BindlessTexture2DHandle firstHandle =
		firstRegisteredTexture->getBindlessHandle();
	ASSERT(
		BindlessResources::Instance->validate(firstHandle) ==
		firstHandle.index)
	delete firstRegisteredTexture;
	ASSERT(texturePool.find(registeredName) == texturePool.end())

	auto* secondRegisteredTexture = new Texture2D(
		registeredName,
		pixel,
		1,
		1,
		{4, 8, 0},
		false,
		bindlessInfo);
	const BindlessTexture2DHandle secondHandle =
		secondRegisteredTexture->getBindlessHandle();
	ASSERT(secondHandle.index == firstHandle.index)
	ASSERT(secondHandle.generation == firstHandle.generation + 1)
	delete secondRegisteredTexture;
	ASSERT(texturePool.find(registeredName) == texturePool.end())
	LOG("Texture bindless lifetime self-test passed")
}
#endif

Texture2D::Texture2D(
	ImageCreator &imageCreator,
	const BindlessTexture2DInfo& bindlessInfo)
{
	imageFormat = imageCreator.imageInfo.format;
	width = imageCreator.imageInfo.extent.width;
	height = imageCreator.imageInfo.extent.height;

	imageCreator.create(resource, imageView);
	registerBindless(bindlessInfo);
}

Texture2D::Texture2D(
	const std::string &name,
	uint8_t *data,
	uint32_t width,
	uint32_t height,
	ImageFormat format,
	bool generateMips,
	const BindlessTexture2DInfo& bindlessInfo)
{
	LOG("loading texture '%s'..", name.c_str())

	imageFormat = getFormatFromMap(format);
	this->width = width;
	this->height = height;

	createTexture2DFromPixelData(
		data, width, height,
		imageFormat,
		(format.numChannels * format.channelDepth / 8),
		generateMips,
		resource,
		imageView);
	texturePool[name] = this;

	NAME_OBJECT(VK_OBJECT_TYPE_IMAGE, resource.image, name)
	NAME_OBJECT(VK_OBJECT_TYPE_IMAGE_VIEW, imageView, name + "_defaultView")
	registerBindless(bindlessInfo);
}

void Texture2D::registerBindless(const BindlessTexture2DInfo& bindlessInfo)
{
	if (!bindlessInfo.registerTexture) return;

	ASSERT(BindlessResources::Instance != nullptr)
	ASSERT(bindlessInfo.samplerInfo.sType == VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO)
	ASSERT(bindlessHandle.index == INVALID_BINDLESS_INDEX)

	bindlessHandle = BindlessResources::Instance->addTexture2D(
		imageView,
		bindlessInfo.samplerInfo);
}

void Texture2D::unregisterBindless()
{
	if (bindlessHandle.index == INVALID_BINDLESS_INDEX) return;

	ASSERT(BindlessResources::Instance != nullptr)
	BindlessResources::Instance->removeTexture2D(bindlessHandle);
	bindlessHandle = {};
}

Texture2D::~Texture2D()
{
	unregisterBindless();
	vkDestroyImageView(Vulkan::Instance->device, imageView, nullptr);
}
