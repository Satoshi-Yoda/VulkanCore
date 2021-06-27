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

	void initRectangle(int x, int y, RectangleData data);

private:
	Ash& ash;
	Mountain& mountain;
	Rocks& rocks;
	Crater& crater;
	Lava& lava;

	vector<RectangleVertex> initQuad(int x, int y, uint32_t w, uint32_t h);
};
