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

enum class GraphicStyle {
	LINE,
	AREA,
};

struct GraphicVertex {
	vec2 pos;
	vec2 texCoord;
};

typedef float GraphicElement;

struct GraphicData {
	vec4 bgColor;
	vec4 lineColor;
	vec2 size;
	float radius = 0.0f;
	float aa = 1.0f;
	float line = 0.25f;
	vector<GraphicElement> points;
};

class GraphicLayout {
public:
	GraphicLayout(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater);
	GraphicLayout(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater, GraphicStyle style);
	~GraphicLayout();

	GraphicLayout(const GraphicLayout&)            = delete;
	GraphicLayout(GraphicLayout&&)                 = delete;
	GraphicLayout& operator=(const GraphicLayout&) = delete;
	GraphicLayout& operator=(GraphicLayout&&)      = delete;

	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

private:
	Ash& ash;
	Mountain& mountain;
	Rocks& rocks;
	Crater& crater;

	GraphicStyle style;

	void createDescriptorSetLayout();
	void createPipeline();
};
