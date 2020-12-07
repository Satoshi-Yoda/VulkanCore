#ifndef LAVA_H
#define LAVA_H

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
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout descriptorSetLayout2;
	VkSampler textureSampler;

	vector<BatchLiveData> batchData;
	vector<Cave> caves;

	size_t addBatch(BatchCreateData& createData);
	void addBatches(vector<BatchCreateData> createDataVector);
	void addCave(Cave& cave);
	void resizeInstanceBuffer(size_t index, vector<Instance> instances, VkCommandBuffer externalCommandBuffer = nullptr);
	// void resizeInstanceBuffers(vector<size_t> indexVector, vector<vector<Instance>> instancesVector);
	void updateInstanceBuffer(size_t index, vector<Instance> instances);
	// void updateInstances(size_t index, vector<Instance> instances, vector<size_t> indexes);

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
	void createDescriptorSetLayout2();

	// TODO make index first parameter
	void establishVertexBuffer2(vector<Vertex> vertices, size_t index, VkCommandBuffer externalCommandBuffer = nullptr);
	void establishInstanceBuffer2(vector<Instance> instances, size_t index, VkCommandBuffer externalCommandBuffer = nullptr);
	void establishTextureVMA(int width, int height, void* pixels, size_t index, VkCommandBuffer externalCommandBuffer = nullptr);
};

#endif