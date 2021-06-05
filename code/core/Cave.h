#pragma once

#include <bitset>
#include <set>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "Crater.h"
#include "Rocks.h"
#include "../state/flag_group.hpp"

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
	VULKAN_ENTITIES,
};

class Cave {
public:
	Cave(Ash& ash);
	~Cave();

	Cave(const Cave&)            = delete;
	Cave(Cave&&)                 = delete;
	Cave& operator=(const Cave&) = delete;
	Cave& operator=(Cave&&)      = delete;

	void setName(string name);
	void setWorkingData(vector<Vertex> vertices, int width, int height, void* pixels);
	void setVulkanEntities(Mountain& mountain, Rocks& rocks, Crater& crater);

	flag_group<CaveAspect> aspects;

	string name;

	vector<Vertex> vertices;
	vector<Instance> instances;
	int width;
	int height;
	void* pixels;
	vector<size_t> vacuum;

	uint32_t vertexCount = 0;
	VkBuffer vertexBuffer;
	VmaAllocation vertexAllocation;
	VmaAllocationInfo vertexInfo;
	uint32_t stagingVertexCount = 0;
	VkBuffer stagingVertexBuffer;
	VmaAllocation stagingVertexAllocation;
	VmaAllocationInfo stagingVertexInfo;

	uint32_t instanceCount = 0;
	VkBuffer instanceBuffer;
	VmaAllocation instanceAllocation;
	VmaAllocationInfo instanceInfo;
	uint32_t stagingInstanceCount = 0;
	VkBuffer stagingInstanceBuffer;
	VmaAllocation stagingInstanceAllocation;
	VmaAllocationInfo stagingInstanceInfo;

	VkImage textureImage;
	VmaAllocation textureAllocation;
	VkImageView textureView;
	VkBuffer stagingTextureBuffer;
	VmaAllocation stagingTextureAllocation;
	VmaAllocationInfo stagingTextureInfo;

	void establish(CaveAspect aspect);
	template <typename... Args>
	void establish(CaveAspect aspect, Args... args) {
		establish(aspect);
		establish(args...);
	}

	// TODO can I use the same "establish" name for this?
	void mount(VkCommandBuffer commandBuffer, CaveAspect aspect);
	template <typename... Args>
	void mount(VkCommandBuffer commandBuffer, CaveAspect aspect, Args... args) {
		mount(commandBuffer, aspect);
		mount(commandBuffer, args...);
	}

	void refresh(CaveAspect aspect);
	template <typename... Args>
	void refresh(CaveAspect aspect, Args... args) {
		refresh(aspect);
		refresh(args...);
	}

	void updateInstances(vector<size_t> indexes);

	void free(CaveAspect aspect);
	template <typename... Args>
	void free(CaveAspect aspect, Args... args) {
		free(aspect);
		free(args...);
	}

private:
	Ash& ash;
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
