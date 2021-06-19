#pragma once

#include <memory>
#include <mutex>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../batch/Batch.h"
#include "../batch/BatchLayout.h"
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

class Batch;

// TODO now lava contains all possible drawable objects in engine. But scene contains only used ones. Maybe it can be merged to single entity.
class Lava {
public:
	Lava(Ash &ash, Mountain &mountain, Rocks &rocks, Crater &crater);
	~Lava();

	VkBuffer uniformBuffer;
	VmaAllocation uniformBuffersAllocation;
	VmaAllocationInfo uniformBuffersAllocationInfo;

	vector<unique_ptr<Batch>> batches;

	void addBatch(unique_ptr<Batch> batch);

private:
	Ash& ash;
	Mountain& mountain;
	Rocks& rocks;
	Crater& crater;

public:
	BatchLayout batchLayout { ash, mountain, rocks, crater };

private:
	void createUniformBuffers();
};
