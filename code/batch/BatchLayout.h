#pragma once

#include <bitset>
#include <set>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../core/Crater.h"
#include "../core/Mountain.h"
#include "../core/Rocks.h"

using std::bitset;
using std::set;
using std::string;
using std::vector;

using glm::vec2;
using glm::vec3;

class Lava;

struct Vertex {
	vec2 pos;
	vec2 texCoord;
};

struct Instance {
	vec2 pos;
};

class BatchLayout {
public:
	BatchLayout(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater);
	~BatchLayout();

	BatchLayout(const BatchLayout&)            = delete;
	BatchLayout(BatchLayout&&)                 = delete;
	BatchLayout& operator=(const BatchLayout&) = delete;
	BatchLayout& operator=(BatchLayout&&)      = delete;

	VkSampler textureSampler;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

private:
	Ash& ash;
	Mountain& mountain;
	Rocks& rocks;
	Crater& crater;

	void createTextureSampler();
	void createDescriptorSetLayout();
	void createPipeline();
};
