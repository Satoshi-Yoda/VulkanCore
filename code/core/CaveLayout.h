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

#include "Crater.h"
#include "Mountain.h"
#include "Rocks.h"

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

class CaveLayout {
public:
	CaveLayout(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater);
	~CaveLayout();

	CaveLayout(const CaveLayout&)            = delete;
	CaveLayout(CaveLayout&&)                 = delete;
	CaveLayout& operator=(const CaveLayout&) = delete;
	CaveLayout& operator=(CaveLayout&&)      = delete;

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
