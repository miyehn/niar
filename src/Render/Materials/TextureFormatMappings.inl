
VkFormat getFormatFromMap(ImageFormat imageFormat)
{
	static std::unordered_map<ImageFormat, VkFormat> formatMap;

	formatMap[{1, 8, 0}] = VK_FORMAT_R8_UNORM;
	formatMap[{1, 16, 0}] = VK_FORMAT_R16_SFLOAT;
	formatMap[{1, 32, 0}] = VK_FORMAT_R32_SFLOAT;
	formatMap[{4, 16, 0}] = VK_FORMAT_R16G16B16A16_SFLOAT;
	formatMap[{4, 8, 0}] = VK_FORMAT_R8G8B8A8_UNORM;
	formatMap[{4, 8, 1}] = VK_FORMAT_R8G8B8A8_SRGB;

	return formatMap[imageFormat];
}

