#pragma once

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "Graphic.h"

using std::vector;

class Lava;

class GraphicMaker {
public:
	GraphicMaker(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava);
	~GraphicMaker();

	GraphicMaker(const GraphicMaker&)            = delete;
	GraphicMaker(GraphicMaker&&)                 = delete;
	GraphicMaker& operator=(const GraphicMaker&) = delete;
	GraphicMaker& operator=(GraphicMaker&&)      = delete;

	void initGraphic(int x, int y, GraphicData data);

private:
	Ash& ash;
	Mountain& mountain;
	Rocks& rocks;
	Crater& crater;
	Lava& lava;

	vector<GraphicVertex> initQuad(int x, int y, uint32_t w, uint32_t h);
};
