#include "Rocks.h"

#include <vector>
#include <fstream>
#include <chrono>

using namespace std;

Rocks::Rocks(Ash &ash, Mountain &mountain) : ash(ash), mountain(mountain) {}

Rocks::~Rocks() {}

VkShaderModule Rocks::createShaderModule(const vector<char>& code) {
	VkShaderModuleCreateInfo createInfo { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	vkCreateShaderModule(mountain.device, &createInfo, nullptr, &shaderModule) >> ash("Failed to create shader module!");

	return shaderModule;
}

void Rocks::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
		VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
		VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = numSamples;
	imageInfo.flags = 0; // TODO Here is sparse images & cube images

	vkCreateImage(mountain.device, &imageInfo, nullptr, &image) >> ash("Failed to create image!");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(mountain.device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	vkAllocateMemory(mountain.device, &allocInfo, nullptr, &imageMemory) >> ash("Failed to allocate image memory!");
	vkBindImageMemory(mountain.device, image, imageMemory, 0)            >> ash("Failed to bind image memory!");
}

VkImageView Rocks::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
	VkImageViewCreateInfo info { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	info.image = image;
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.format = format;
	info.subresourceRange.aspectMask = aspectFlags;
	info.subresourceRange.baseMipLevel = 0;
	info.subresourceRange.levelCount = mipLevels;
	info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount = 1;

	VkImageView imageView;
	vkCreateImageView(mountain.device, &info, nullptr, &imageView) >> ash("Failed to create image view!");

	return imageView;
}

void Rocks::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	} else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else {
		throw invalid_argument("Unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(commandBuffer);
}

uint32_t Rocks::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(mountain.physicalDevice, &memProperties);

	uint32_t result = UINT32_MAX;

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			result = i;
			break;
		}
	}

	(result != UINT32_MAX) >> ash("Failed to find suitable memory type!");

	// printf("Chosen %d memory type\n", result);

	return result;
}

VkCommandBuffer Rocks::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = mountain.commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer tempCommandBuffer;
	vkAllocateCommandBuffers(mountain.device, &allocInfo, &tempCommandBuffer);

	VkCommandBufferBeginInfo beginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(tempCommandBuffer, &beginInfo);
	return tempCommandBuffer;
}

void Rocks::endSingleTimeCommands(VkCommandBuffer tempCommandBuffer) {
	vkEndCommandBuffer(tempCommandBuffer);

	VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &tempCommandBuffer;

	VkFenceCreateInfo fenceCreateInfo { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	VkFence fence;
	vkCreateFence(mountain.device, &fenceCreateInfo, nullptr, &fence) >> ash("Failed to create fence for waiting single time command buffer!");
	vkQueueSubmit(mountain.queue, 1, &submitInfo, fence)              >> ash("Failed to submit single time command buffer!");
	vkWaitForFences(mountain.device, 1, &fence, VK_TRUE, UINT32_MAX);
	vkDestroyFence(mountain.device, fence, nullptr);

	// simple implementation
	// vkQueueSubmit(mountain.queue, 1, &submitInfo, VK_NULL_HANDLE) >> ash("Failed to submit single time command buffer!");
	// vkQueueWaitIdle(mountain.queue);

	vkFreeCommandBuffers(mountain.device, mountain.commandPool, 1, &tempCommandBuffer);
}

void Rocks::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	VkBufferCreateInfo bufferInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.queueFamilyIndexCount = 0;
	bufferInfo.pQueueFamilyIndices = nullptr;

	vkCreateBuffer(mountain.device, &bufferInfo, nullptr, &buffer) >> ash("Failed to create buffer!");

	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(mountain.device, buffer, &requirements);

	VkMemoryAllocateInfo allocInfo { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = requirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, properties);

	vkAllocateMemory(mountain.device, &allocInfo, nullptr, &bufferMemory) >> ash("Failed to allocate buffer memory!");
	vkBindBufferMemory(mountain.device, buffer, bufferMemory, 0)          >> ash("Failed to bind buffer memory!");
}

void Rocks::copyBufferToBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size, VkAccessFlags resultAccessFlags) {
	// auto start = chrono::high_resolution_clock::now();

	VkCommandBuffer tempCommandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(tempCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	VkBufferMemoryBarrier barrier { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = resultAccessFlags;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = dstBuffer;
	barrier.offset = 0;
	barrier.size = VK_WHOLE_SIZE;

	vkCmdPipelineBarrier(tempCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

	endSingleTimeCommands(tempCommandBuffer);

	// auto finish = chrono::high_resolution_clock::now();
	// auto delay = chrono::duration_cast<chrono::duration<double>>(finish - start).count();
	// float speed = static_cast<float>(size) / (1 << 30) / delay;
	// printf("Copied b2b %d MB in %.3fs at %.2f GB/s\n", size / (1 << 20), delay, speed);
}

void Rocks::copyBufferToBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, vector<VkBufferCopy> regions, VkAccessFlags resultAccessFlags) {
	// auto start = chrono::high_resolution_clock::now();

	VkCommandBuffer tempCommandBuffer = beginSingleTimeCommands();

	vkCmdCopyBuffer(tempCommandBuffer, srcBuffer, dstBuffer, static_cast<uint32_t>(regions.size()), regions.data());

	VkBufferMemoryBarrier barrier { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = resultAccessFlags;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = dstBuffer;
	barrier.offset = 0;
	barrier.size = VK_WHOLE_SIZE;

	vkCmdPipelineBarrier(tempCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

	endSingleTimeCommands(tempCommandBuffer);

	// auto finish = chrono::high_resolution_clock::now();
	// auto delay = chrono::duration_cast<chrono::duration<double>>(finish - start).count();
	// float speed = static_cast<float>(size) / (1 << 30) / delay;
	// printf("Copied b2b %d MB in %.3fs at %.2f GB/s\n", size / (1 << 20), delay, speed);
}

void Rocks::copyDataToBuffer(const void* srcPointer, VkDeviceMemory& bufferMemory, size_t size) {
	void* dstPointer;
	vkMapMemory(mountain.device, bufferMemory, 0, VK_WHOLE_SIZE, 0, &dstPointer);

		// auto start = chrono::high_resolution_clock::now();
		memcpy(dstPointer, srcPointer, size);
		// auto finish = chrono::high_resolution_clock::now();
		// auto delay = chrono::duration_cast<chrono::duration<double>>(finish - start).count();
		// float speed = static_cast<float>(size) / (1 << 30) / delay;
		// printf("Copied d2b %d MB in %.3fs at %.2f GB/s\n", size / (1 << 20), delay, speed);

		VkMappedMemoryRange flushRange { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
		flushRange.memory = bufferMemory;
		flushRange.offset = 0;
		flushRange.size = VK_WHOLE_SIZE;
		vkFlushMappedMemoryRanges(mountain.device, 1, &flushRange);
	vkUnmapMemory(mountain.device, bufferMemory);
}

void Rocks::copyBufferToImage(VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(commandBuffer);
}

vector<char> Rocks::readFile(const string& filename) {
	ifstream file(filename, ios::ate | ios::binary);
	file.is_open() >> ash("Failed to open shader file!"); // TODO printf with ash

	size_t fileSize = static_cast<size_t>(file.tellg());
	vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}
