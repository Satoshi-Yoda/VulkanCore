#ifndef ROCKS_H
#define ROCKS_H

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Ash.h"
#include "Mountain.h"

using std::vector;
using std::string;

class Rocks {
public:
	Rocks(Ash &ash, Mountain &mountain);
	~Rocks();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	VkShaderModule createShaderModule(const vector<char>& code);
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
			VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
			VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void copyBufferToBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size, VkAccessFlags resultAccessFlags);
	void copyBufferToBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, vector<VkBufferCopy> regions, VkAccessFlags resultAccessFlags);
	void copyDataToBuffer(const void* srcPointer, VkDeviceMemory& bufferMemory, size_t size);
	void copyBufferToImage(VkBuffer& buffer, VkImage& image, uint32_t width, uint32_t height);

	vector<char> readFile(const string& filename);

private:
	Ash& ash;
	Mountain& mountain;
};

#endif