#include "Render/Vulkan/VulkanUtils.h"
#include "ShaderBindingTable.h"
#include "Utils/myn/Misc.h"

ShaderBindingTable::ShaderBindingTable(VkPipeline pipeline, uint32_t numHitShaders, uint32_t numMissShaders,
									   uint32_t numCallableShaders)
{
	// in SBT, shader be stored like: [raygen][hit, hit, ...][miss, miss, ...][callable, callabla, ...]

	const uint32_t baseAlignment = Vulkan::Instance->shaderGroupBaseAlignment;
	const uint32_t handleSize = Vulkan::Instance->shaderGroupHandleSize;
	const uint32_t handleAlignment = Vulkan::Instance->shaderGroupHandleAlignment;

	uint32_t totalHandlesCount = 1 + numHitShaders + numMissShaders + numCallableShaders;
	uint32_t handleSizeAligned = myn::aligned_size(handleAlignment, handleSize);

	raygenRegion = {
		// for raygen, stride must equal to size
		.stride = myn::aligned_size(baseAlignment, handleSizeAligned),
		.size = myn::aligned_size(baseAlignment, handleSizeAligned)
	};
	hitRegion = {
		.stride = handleSizeAligned,
		.size = myn::aligned_size(baseAlignment, handleSizeAligned * numHitShaders)
	};
	missRegion = {
		.stride = handleSizeAligned,
		.size = myn::aligned_size(baseAlignment, handleSizeAligned * numMissShaders)
	};
	callableRegion = {
		.stride = handleSizeAligned,
		.size = myn::aligned_size(baseAlignment, handleSizeAligned * numCallableShaders)
	};

	// get shader handles
	uint32_t dataSize = totalHandlesCount * handleSize;
	std::vector<uint8_t> handles(dataSize); // tightly packed handles
	EXPECT(Vulkan::Instance->fn_vkGetRayTracingShaderGroupHandlesKHR(Vulkan::Instance->device, pipeline, 0, totalHandlesCount, dataSize, handles.data()), VK_SUCCESS)

	VkDeviceSize sbtSize = raygenRegion.size + hitRegion.size + missRegion.size + callableRegion.size;
	shaderBindingTable = VmaBuffer(
		&Vulkan::Instance->memoryAllocator,
		sbtSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU);
	NAME_OBJECT(VK_OBJECT_TYPE_BUFFER, shaderBindingTable.getBufferInstance(), "shader binding table")

	std::vector<uint8_t> alignedHandles(sbtSize, 0);
	const uint32_t raygenStart = 0;
	const uint32_t hitStart = raygenRegion.size;
	const uint32_t missStart = hitStart + hitRegion.size;
	const uint32_t callableStart = missStart + missRegion.size;
	// rgen
	memcpy(&alignedHandles[raygenStart], &handles[0], handleSize);
	// hit
	for (auto i = 0; i < numHitShaders; i++)
	{
		memcpy(
			&alignedHandles[hitStart + i * handleSizeAligned],
			&handles[(1 + i) * handleSize],
			handleSize);
	}
	// miss
	for (auto i = 0; i < numMissShaders; i++)
	{
		memcpy(
			&alignedHandles[missStart + i * handleSizeAligned],
			&handles[(1 + numHitShaders + i) * handleSize],
			handleSize);
	}
	// callable
	for (auto i = 0; i < numCallableShaders; i++)
	{
		memcpy(
			&alignedHandles[callableStart + i * handleSizeAligned],
			&handles[(1 + numHitShaders + numMissShaders + i) * handleSize],
			handleSize);
	}

	shaderBindingTable.writeData(alignedHandles.data(), alignedHandles.size());

	// also get the device addresses
	VkBufferDeviceAddressInfo addressInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = shaderBindingTable.getBufferInstance()
	};
	VkDeviceAddress address = vkGetBufferDeviceAddress(Vulkan::Instance->device, &addressInfo);
	raygenRegion.deviceAddress = address;
	// do the below addresses not even get used...
	hitRegion.deviceAddress = raygenRegion.deviceAddress + raygenRegion.size;
	missRegion.deviceAddress = hitRegion.deviceAddress + hitRegion.size;
	callableRegion.deviceAddress = missRegion.deviceAddress + missRegion.size;
}
