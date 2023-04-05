#pragma once

#include <memory>
#include <mutex>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vk_mem_alloc.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../batch/Batch.h"
#include "../batch/BatchLayout.h"
#include "../graphic/Graphic.h"
#include "../graphic/GraphicLayout.h"
#include "../rectangle/Rectangle.h"
#include "../rectangle/RectangleLayout.h"
#include "Crater.h"
#include "Mountain.h"
#include "Rocks.h"

using std::shared_ptr;
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

class Batch;
class Graphic;
class Rectangle;

// TODO now lava contains all possible drawable objects in engine. But scene contains only used ones. Maybe it can be merged to single entity.
class Lava {
public:
	Lava(Ash &ash, Mountain &mountain, Rocks &rocks, Crater &crater);
	~Lava();

	VkBuffer uniformBuffer;
	VmaAllocation uniformBuffersAllocation;
	VmaAllocationInfo uniformBuffersAllocationInfo;

	vector<unique_ptr<Batch>> batches;
	vector<shared_ptr<Graphic>> graphics;
	vector<shared_ptr<Rectangle>> rectangles;

	void addBatch(unique_ptr<Batch> batch);
	void addRectangle(shared_ptr<Rectangle> rectangle);
	void addGraphic(shared_ptr<Graphic> graphic);

private:
	Ash& ash;
	Mountain& mountain;
	Rocks& rocks;
	Crater& crater;

public:
	BatchLayout batchLayout { ash, mountain, rocks, crater };
	RectangleLayout rectangleLayout { ash, mountain, rocks, crater };
	GraphicLayout lineGraphicLayout { ash, mountain, rocks, crater, GraphicStyle::LINE };
	GraphicLayout areaGraphicLayout { ash, mountain, rocks, crater, GraphicStyle::AREA };

private:
	void createUniformBuffers();
};
