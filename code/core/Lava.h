#pragma once

#include <memory>
#include <mutex>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "Cave.h"
#include "Crater.h"
#include "Mountain.h"
#include "Rocks.h"

using std::unique_ptr;
using std::vector;

class Lava {
public:
	Lava(Ash &ash, Mountain &mountain, Rocks &rocks, Crater &crater);
	~Lava();

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkSampler textureSampler;
	VkDescriptorSetLayout descriptorSetLayout;

	VkBuffer uniformBuffer;
	VmaAllocation uniformBuffersAllocation;
	VmaAllocationInfo uniformBuffersAllocationInfo;

	vector<unique_ptr<Cave>> caves;

	void addCave(unique_ptr<Cave> cave);

private:
	Ash& ash;
	Mountain& mountain;
	Rocks& rocks;
	Crater& crater;

	int mipLevels = 1;

	void createRenderPass();
	void createPipeline();
	void createTextureSampler();
	void createDescriptorSetLayout();

	void createUniformBuffers();
};
