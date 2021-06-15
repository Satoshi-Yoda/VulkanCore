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
#include "CaveLayout.h"
#include "Crater.h"
#include "Mountain.h"
#include "Rocks.h"

using std::unique_ptr;
using std::vector;

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::radians;

struct UniformBufferObject {
    vec2 scale;
    vec2 shift;
};

class Cave;

class Lava {
public:
	Lava(Ash &ash, Mountain &mountain, Rocks &rocks, Crater &crater);
	~Lava();

	// TODO this is drawing-primitive-specific, so maybe move it to Cave static or something?
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

	CaveLayout caveLayout { ash, mountain, rocks, crater };

	void createPipeline();
	void createTextureSampler();
	void createDescriptorSetLayout();

	void createUniformBuffers();
};
