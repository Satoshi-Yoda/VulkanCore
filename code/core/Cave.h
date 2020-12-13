#ifndef CAVE_H
#define CAVE_H

#include <bitset>
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
using std::bitset;
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
	NONE = 0,
	WORKING_VERTICES  = 1 << 0,
	WORKING_INSTANCES = 1 << 1,
	WORKING_TEXTURE   = 1 << 2,
	STAGING_VERTICES  = 1 << 3,
	STAGING_INSTANCES = 1 << 4,
	STAGING_TEXTURE   = 1 << 5,
	LIVE_VERTICES     = 1 << 6,
	LIVE_INSTANCES    = 1 << 7,
	LIVE_TEXTURE      = 1 << 8,
	VULKAN_ENTITIES   = 1 << 9,
};

inline constexpr CaveAspects operator|(CaveAspects a, CaveAspects b) {
	return static_cast<CaveAspects>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

inline constexpr CaveAspects operator&(CaveAspects a, CaveAspects b) {
	return static_cast<CaveAspects>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

inline constexpr CaveAspects& operator|=(CaveAspects& a, CaveAspects b) {
	a = static_cast<CaveAspects>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
	return a;
}

inline constexpr CaveAspects& operator&=(CaveAspects& a, CaveAspects b) {
	a = static_cast<CaveAspects>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
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
	Cave();
	~Cave();

	Cave(const Cave&)            = delete;
	Cave(Cave&&)                 = delete;
	Cave& operator=(const Cave&) = delete;
	Cave& operator=(Cave&&)      = delete;

	void setName(string name);
	void setWorkingData(vector<Vertex> vertices, int width, int height, void* pixels);
	void setVulkanEntities(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater);

	CaveAspects aspects;

	string name;

	vector<Vertex> vertices;
	vector<Instance> instances;
	int width;
	int height;
	void* pixels;
	vector<size_t> vacuum;

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
	void refresh(CaveAspects aspects);
	void updateInstances(vector<size_t> indexes);
	void free(CaveAspects aspects);

	bool canBeDrawn();

private:
	Ash* ash;
	Mountain* mountain;
	Rocks* rocks;
	Crater* crater;

	void establishStagingVertices();
	void establishStagingInstances();
	void refreshStagingInstances();
	void establishStagingTexture();
	void establishLiveVertices(VkCommandBuffer externalCommandBuffer = nullptr);
	void establishLiveInstances(VkCommandBuffer externalCommandBuffer = nullptr);
	void refreshLiveInstances(VkCommandBuffer externalCommandBuffer = nullptr);
	void establishLiveTexture(VkCommandBuffer externalCommandBuffer = nullptr);

	void freeStagingVertices();
	void freeStagingInstances();
	void freeStagingTexture();
	void freeLiveVertices();
	void freeLiveInstances();
	void freeLiveTexture();
};
#endif