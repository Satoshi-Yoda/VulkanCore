#pragma once

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "Rectangle.h"

using std::vector;

class Lava;

class RectangleMaker {
public:
	RectangleMaker(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava);
	~RectangleMaker();

	RectangleMaker(const RectangleMaker&)            = delete;
	RectangleMaker(RectangleMaker&&)                 = delete;
	RectangleMaker& operator=(const RectangleMaker&) = delete;
	RectangleMaker& operator=(RectangleMaker&&)      = delete;

	void initRectangle();

private:
	Ash& ash;
	Mountain& mountain;
	Rocks& rocks;
	Crater& crater;
	Lava& lava;

	vector<Vertex> initQuad(uint32_t w, uint32_t h);
};
