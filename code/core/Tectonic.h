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

	VkCommandBuffer commandBuffer;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkFence fence;
	VkFramebuffer framebuffer;
	VkBuffer uniformBuffer;
	VmaAllocation uniformBuffersAllocation;
	VmaAllocationInfo uniformBuffersAllocationInfo;
	vector<VkDescriptorSet> descriptorSets; // TODO with removal of in flight frames maybe this can go to cave?

	void createInFlightResources();
	void createUniformBuffers();
	void resizeDescriptorSets(size_t size);
	void updateInFlightUniformBuffer();
	void updateDescriptorSet(size_t textureIndex, VkImageView& imageView);
	void prepareFrame(uint32_t craterIndex);
};
