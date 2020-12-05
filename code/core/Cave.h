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
#include "Rocks.h"
#include "Crater.h"

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

enum class CaveAspects : uint16_t {
	WORKING_VERTICES  = 1 << 0,
	WORKING_INSTANCES = 1 << 1,
	WORKING_TEXTURE   = 1 << 2,
	STAGING_VERTICES  = 1 << 3,
	STAGING_INSTANCES = 1 << 4,
	STAGING_TEXTURE   = 1 << 5,
	LIVE_VERTICES     = 1 << 6,
	LIVE_INSTANCES    = 1 << 7,
	LIVE_TEXTURE      = 1 << 8,
};

inline constexpr CaveAspects operator|(CaveAspects a, CaveAspects b) {
	return static_cast<CaveAspects>(a | b);
}

inline constexpr CaveAspects operator&(CaveAspects a, CaveAspects b) {
	return static_cast<CaveAspects>(a & b);
}

inline constexpr CaveAspects& operator|=(CaveAspects& a, CaveAspects b) {
	a = static_cast<CaveAspects>(a | b);
	return a;
}

inline constexpr CaveAspects& operator&=(CaveAspects& a, CaveAspects b) {
	a = static_cast<CaveAspects>(a & b);
	return a;
}

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
	Cave(Rocks& rocks, Crater& crater, vector<Vertex> vertices, int width, int height, void* pixels);
	~Cave();

	CaveAspects aspects;

	string name;

	vector<Vertex> vertices;
	vector<Instance> instances;
	int width;
	int height;
	void* pixels;
	vector<size_t> unused;

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

	bool has(CaveAspects aspects);
	void establish(CaveAspects aspects);
	void free(CaveAspects aspects);

	bool canBeDrawn();

private:
	Rocks& rocks;
	Crater& crater;

	void establishStagingVertices();
	void establishStagingInstances();
	void establishStagingTexture();
	void establishLiveVertices(VkCommandBuffer externalCommandBuffer);
	void establishLiveInstances(VkCommandBuffer externalCommandBuffer);
	void establishLiveTexture(VkCommandBuffer externalCommandBuffer);
};

#endif