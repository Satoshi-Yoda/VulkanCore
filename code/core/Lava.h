#ifndef LAVA_H
#define LAVA_H

#include <memory>
#include <mutex>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "Cave.h"
#include "Crater.h"
#include "Mountain.h"
#include "Rocks.h"

using std::array;
using std::mutex;
using std::unique_ptr;
using std::vector;
using glm::vec2;
using glm::vec3;

// TODO
// Cave
// store batch all data in 1 class with state
// make establish(), freeCPU(), freeGPU(), etc methods
// store lavaObjectId (just index), stats in that class
// aspects: working, staging, live

struct BatchCreateData {
	vector<Vertex> vertices;
	vector<Instance> instances;
	int width;
	int height;
	void* pixels;
	vector<size_t> free;
};

struct BatchLiveData {
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

class Lava {
public:
	Lava(Ash &ash, Mountain &mountain, Rocks &rocks, Crater &crater);
	~Lava();

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkSampler textureSampler;
	VkDescriptorSetLayout descriptorSetLayout;

	vector<unique_ptr<Cave>> caves;

	void addCave(unique_ptr<Cave> cave);

private:
	Ash& ash;
	Mountain& mountain;
	Rocks& rocks;
	Crater& crater;

	int mipLevels = 1;

	void createRenderPass();
	void createPipeline();
	void createTextureSampler();
	void createDescriptorSetLayout();
};

#endif