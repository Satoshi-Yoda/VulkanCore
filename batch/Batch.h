#pragma once

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
#include "../core/Lava.h"
#include "../core/Rocks.h"
#include "../state/flag_group.hpp"
#include "BatchLayout.h"

using std::bitset;
using std::set;
using std::string;
using std::vector;

using glm::vec2;
using glm::vec3;

class Lava;

enum class BatchAspect {
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

class Batch {
public:
	Batch(Ash& ash);
	~Batch();

	Batch(const Batch&)            = delete;
	Batch(Batch&&)                 = delete;
	Batch& operator=(const Batch&) = delete;
	Batch& operator=(Batch&&)      = delete;

	void setName(string name);
	void setWorkingData(vector<Vertex> vertices, int width, int height, void* pixels);
	void setVulkanEntities(Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava);

	flag_group<BatchAspect> aspects;

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

	VkDescriptorSet descriptorSet;

	void establish(BatchAspect aspect);
	template <typename... Args>
	void establish(BatchAspect aspect, Args... args) {
		establish(aspect);
		establish(args...);
	}

	void establish(VkCommandBuffer cb, BatchAspect aspect);
	template <typename... Args>
	void establish(VkCommandBuffer cb, BatchAspect aspect, Args... args) {
		establish(cb, aspect);
		establish(cb, args...);
	}

	void refresh(BatchAspect aspect);
	template <typename... Args>
	void refresh(BatchAspect aspect, Args... args) {
		refresh(aspect);
		refresh(args...);
	}

	void updateInstances(vector<size_t> indexes);

	void createDescriptorSet();

	void free(BatchAspect aspect);
	template <typename... Args>
	void free(BatchAspect aspect, Args... args) {
		free(aspect);
		free(args...);
	}

private:
	Ash& ash;
	Mountain* mountain = nullptr;
	Rocks* rocks       = nullptr;
	Crater* crater     = nullptr;
	Lava* lava         = nullptr;

	void establishStagingVertices();
	void establishStagingInstances();
	void refreshStagingInstances();
	void establishStagingTexture();
	void establishLiveVertices (VkCommandBuffer externalCommandBuffer = nullptr);
	void establishLiveInstances(VkCommandBuffer externalCommandBuffer = nullptr);
	void refreshLiveInstances  (VkCommandBuffer externalCommandBuffer = nullptr);
	void establishLiveTexture  (VkCommandBuffer externalCommandBuffer = nullptr);

	void freeStagingVertices();
	void freeStagingInstances();
	void freeStagingTexture();
	void freeLiveVertices();
	void freeLiveInstances();
	void freeLiveTexture();
};
