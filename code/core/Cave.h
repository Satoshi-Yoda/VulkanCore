#ifndef CAVE_H
#define CAVE_H

#include <set>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

// #include "Mountain.h"
// #include "Rocks.h"
// #include "Crater.h"

using glm::vec2;
using glm::vec3;
using std::set;
using std::string;
using std::vector;

struct Vertex {
	vec2 pos;
	vec2 texCoord;
};

struct Instance {
	vec2 pos;
};

enum class CaveAspect {
	WORKING_VERTICES,
	WORKING_INSTANCES,
	WORKING_TEXTURE,
	STAGING_VERTICES,
	STAGING_INSTANCES,
	STAGING_TEXTURE,
	LIVE_VERTICES,
	LIVE_INSTANCES,
	LIVE_TEXTURE,
};

// TODO
// Cave
// store batch all data in 1 class with state
// make establish(), freeCPU(), freeGPU(), etc methods
// store lavaObjectId (just index), stats in that class
// aspects: working, staging, live

// TODO
// Batcher does not store anything,
// after Batcher::load() working-caves appears in lava
// after Batcher::establish() staging & live caves aspects established & freed

class Cave {
public:
	Cave();
	~Cave();

	set<CaveAspect> aspects;

	string name;

	vector<Vertex> vertices;
	vector<Instance> instances;
	int width;
	int height;
	void* pixels;
	vector<size_t> free;

	VkBuffer vertexBuffer;
	VmaAllocation vertexAllocation;
	VmaAllocationInfo vertexInfo;
	uint32_t vertexCount;
	VkBuffer stagingVertexBuffer;
	VmaAllocation stagingVertexAllocation;
	VmaAllocationInfo stagingVertexInfo;

	VkBuffer instanceBuffer;
	VmaAllocation instanceAllocation;
	VmaAllocationInfo instanceInfo;
	uint32_t instanceCount;
	VkBuffer stagingInstanceBuffer;
	VmaAllocation stagingInstanceAllocation;
	VmaAllocationInfo stagingInstanceInfo;

	VkImage textureImage;
	VmaAllocation textureAllocation;
	VkImageView textureView;
	VkBuffer stagingTextureBuffer;
	VmaAllocation stagingTextureAllocation;
	VmaAllocationInfo stagingTextureInfo;
};

#endif