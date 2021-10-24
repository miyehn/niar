#include "Texture.h"
#include <stb_image/stb_image.h>
#include "Render/Vulkan/VulkanUtils.h"

std::unordered_map<std::string, Texture *> Texture::pool;

void Texture::cleanup()
{
	for (auto & it : Texture::pool)
	{
		delete it.second;
	}
}

Texture *Texture::get(const std::string &path)
{
	auto it = Texture::pool.find(path);
	if (it != Texture::pool.end()) return (*it).second;

	WARN("retrieving texture '%s' before it is added to the pool. Trying to load from file with default settings (SRGB)..",
		 path.c_str())
	auto new_texture = new Texture2D(path);
	Texture::pool[path] = new_texture;
	return new_texture;
}

Texture::~Texture()
{
	vmaDestroyImage(Vulkan::Instance->memoryAllocator, resource.image, resource.allocation);
}

//--------

void createTexture2DFromPixelData(
	uint8_t *pixels,
	uint32_t width,
	uint32_t height,
	VkFormat imageFormat,
	uint32_t pixelSize,
	VmaAllocatedImage &outResource,
	VkImageView &outImageView)
{
	VkDeviceSize imageSize = width * height * pixelSize;
	VmaBuffer stagingBuffer(&Vulkan::Instance->memoryAllocator, imageSize,
							VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	stagingBuffer.writeData(pixels);

	VkExtent3D imageExtent = {
		.width = static_cast<uint32_t>(width),
		.height = static_cast<uint32_t>(height),
		.depth = 1
	};

	VkImageCreateInfo imgInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = imageFormat,
		.extent = imageExtent,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
	};

	VmaAllocationCreateInfo imgAllocInfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY
	};

	vmaCreateImage(Vulkan::Instance->memoryAllocator, &imgInfo, &imgAllocInfo, &outResource.image, &outResource.allocation, nullptr);

	Vulkan::Instance->immediateSubmit([&](VkCommandBuffer cmdbuf)
	{
		// image layout
		auto transferLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		auto shaderReadLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// barrier the image into the transfer-receive layout
		vk::insertImageBarrier(
			cmdbuf,
			outResource.image,
			{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,1},
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			transferLayout);

		// do the transfer
		VkBufferImageCopy copyRegion = {
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1
			},
			.imageOffset = {0, 0, 0},
			.imageExtent = imageExtent
		};
		vkCmdCopyBufferToImage(
			cmdbuf,
			stagingBuffer.getBufferInstance(),
			outResource.image,
			transferLayout,
			1,
			&copyRegion);

		//barrier it again into shader readonly optimal layout
		vk::insertImageBarrier(
			cmdbuf,
			outResource.image,
			{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,1},
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			transferLayout,
			shaderReadLayout);
	});

	stagingBuffer.release();

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
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	EXPECT(vkCreateImageView(Vulkan::Instance->device, &viewInfo, nullptr, &outImageView), VK_SUCCESS)
}

bool operator==(const ImageFormat& f1, const ImageFormat& f2)
{
	return f1.numChannels==f2.numChannels && f1.channelDepth==f2.channelDepth && f1.SRGB==f2.SRGB;
}
namespace std
{
	template<> struct hash<ImageFormat>
	{
		std::size_t operator()(const ImageFormat &format) const noexcept
		{
			return
			    hash<int>{}(format.numChannels << 0) ^
				hash<int>{}(format.channelDepth << 8) ^
				hash<int>{}(format.SRGB << 16);
		}
	};
}

Texture2D::Texture2D(const std::string &path, ImageFormat textureFormat)
{
#ifdef DEBUG
	auto it = Texture::pool.find(path);
	if (it != Texture::pool.end()) WARN("trying to load texture '%s' that's already in the pool. Overriding..", path.c_str())
	EXPECT(textureFormat.channelDepth % 8, 0)
#endif

	int native_channels;
	int iwidth, iheight;
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

	LOG("load texture w %d h %d channels %d", iwidth, iheight, native_channels)
	EXPECT(pixels != nullptr, true)

	std::unordered_map<ImageFormat, VkFormat> formatMap;
	formatMap[{1, 16, 0}] = VK_FORMAT_R16_SFLOAT;
	formatMap[{1, 32, 0}] = VK_FORMAT_R32_SFLOAT;
	formatMap[{4, 16, 0}] = VK_FORMAT_R16G16B16A16_SFLOAT;
	formatMap[{4, 8, 1}] = VK_FORMAT_R8G8B8A8_SRGB; // since {3, 8, 1} is not valid
	// TODO: move to elsewhere?

	imageFormat = formatMap[textureFormat];

	width = iwidth;
	height = iheight;
	num_slices = 1;

	uint32_t pixelSize = textureFormat.numChannels * (textureFormat.channelDepth / 8);
	createTexture2DFromPixelData(pixels, width, height, imageFormat, pixelSize,resource,imageView);

	stbi_image_free(pixels);

	Texture::pool[path] = this;
}

void Texture2D::createDefaultTextures()
{
	static bool createdDefaultTextures = false;

	if (createdDefaultTextures)
	{
		WARN("Trying to re-create default textures. Skipping..")
		return;
	}

	auto* whiteTexture = new Texture2D();
	whiteTexture->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
	whiteTexture->num_slices = 1;
	whiteTexture->width = 1;
	whiteTexture->height = 1;
	uint8_t whitePixel[] = {1, 1, 1, 1};
	createTexture2DFromPixelData(
		whitePixel, 1, 1,
		VK_FORMAT_R8G8B8A8_SRGB,
		4,
		whiteTexture->resource,
		whiteTexture->imageView);
	Texture::pool["_white"] = whiteTexture;

	// TODO: more
}

Texture2D::~Texture2D()
{
	vkDestroyImageView(Vulkan::Instance->device, imageView, nullptr);
}

Texture2D::Texture2D(ImageCreator &imageCreator)
{
	imageFormat = imageCreator.imageInfo.format;
	width = imageCreator.imageInfo.extent.width;
	height = imageCreator.imageInfo.extent.height;
	num_slices = imageCreator.imageInfo.extent.depth;

	imageCreator.create(resource, imageView);
	if (imageCreator.debugName.length() > 0)
	{
		NAME_OBJECT(VK_OBJECT_TYPE_IMAGE, resource.image, imageCreator.debugName)
		NAME_OBJECT(VK_OBJECT_TYPE_IMAGE_VIEW, imageView, imageCreator.debugName + "_defaultView")
	}
}
