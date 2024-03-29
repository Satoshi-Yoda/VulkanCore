#pragma once

#include <array>
#include <bitset>
#include <set>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vk_mem_alloc.h>

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
using glm::vec4;

class Lava;

struct RectangleVertex {
	vec2 pos;
	vec2 texCoord;
};

struct RectangleData {
	vec4 color;
	vec2 size;
	float radius = 0.0f;
	float aa = 1.0f;
};

class RectangleLayout {
public:
	RectangleLayout(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater);
	~RectangleLayout();

	RectangleLayout(const RectangleLayout&)            = delete;
	RectangleLayout(RectangleLayout&&)                 = delete;
	RectangleLayout& operator=(const RectangleLayout&) = delete;
	RectangleLayout& operator=(RectangleLayout&&)      = delete;

	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

private:
	Ash& ash;
	Mountain& mountain;
	Rocks& rocks;
	Crater& crater;

	void createDescriptorSetLayout();
	void createPipeline();
};
