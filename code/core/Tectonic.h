#pragma once

#include <vector>
#include <array>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vk_mem_alloc.h"

#include "Mountain.h"
#include "Crater.h"
#include "Lava.h"

using std::vector;
using std::array;

class Tectonic {
public:
	Tectonic(Ash &ash, Mountain &mountain, Rocks &rocks, Crater &crater, Lava &lava);
	~Tectonic();

	void drawFrame();

private:
	Ash& ash;
	Mountain& mountain;
	Rocks &rocks;
	Crater& crater;
	Lava &lava;

	// TODO remove IN_FLIGHT_FRAMES (always use 1)
	// const static int IN_FLIGHT_FRAMES = 1;
	// int inFlightIndex = 0;

	VkCommandBuffer commandBuffer;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence fence;
	VkFramebuffer framebuffer;
	VkBuffer uniformBuffer;
	VmaAllocation uniformBuffersAllocation;
	VmaAllocationInfo uniformBuffersAllocationInfo;
	vector<VkDescriptorSet> descriptorSets;

	void createInFlightResources();
	void createUniformBuffers();
	void resizeDescriptorSets(size_t size);
	void updateInFlightUniformBuffer();
	void updateDescriptorSet(size_t textureIndex, VkImageView& imageView);
	void prepareFrame(uint32_t craterIndex);
};
