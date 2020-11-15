#ifndef LAVA_H
#define LAVA_H

#include <mutex>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "Mountain.h"
#include "Rocks.h"
#include "Crater.h"

using std::array;
using std::mutex;
using std::vector;
using glm::vec2;
using glm::vec3;

// TODO maybe move Vertex into separate file
struct Vertex {
	vec2 pos;
	vec2 texCoord;
};

struct Instance {
	vec2 pos;
};

// TODO
// store batch all data in 1 class with state
// make establish(), freeCPU(), freeGPU(), etc methods
// store lavaObjectId, stats in that class

struct BatchCreateData {
	vector<Vertex> vertices;
	vector<Instance> instances;
	int width;
	int height;
	void* pixels;
};

struct BatchLiveData {
	VkBuffer vertexBuffer;
	VmaAllocation vertexAllocation;
	uint32_t vertexCount;

	VkBuffer instanceBuffer;
	VmaAllocation instanceAllocation;
	VmaAllocationInfo instanceAllocationInfo;
	uint32_t instanceCount;

	VkBuffer stagingInstanceBuffer;
	VmaAllocation stagingInstanceAllocation;
	VmaAllocationInfo stagingInstanceAllocationInfo;

	VkImage textureImage;
	VmaAllocation textureAllocation;
	VkImageView textureView;
};

class Lava {
public:
	Lava(Ash &ash, Mountain &mountain, Rocks &rocks, Crater &crater);
	~Lava();

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	vector<VkBuffer> vertexBuffers;
	vector<uint32_t> vertexBufferSizes;
	vector<VkBuffer> instanceBuffers;
	vector<uint32_t> instanceBufferSizes;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout descriptorSetLayout2;

	vector<VkImageView> textureImageViews;
	VkSampler textureSampler;

	vector<BatchLiveData> batchData;

	size_t addObject(vector<Vertex> vertices, vector<Instance> instances, int width, int height, void* pixels);
	size_t addBatch(BatchCreateData& createData);
	void addBatches(vector<BatchCreateData> createDataVector);
	// void updateVertexBuffer(size_t id, vector<Vertex> vertices);
	void updateInstanceBuffer(size_t id, vector<Instance> instances);
	void updateInstances(size_t id, vector<Instance> instances, vector<size_t> indexes);

private:
	Ash& ash;
	Mountain& mountain;
	Rocks& rocks;
	Crater& crater;

	const string TEXTURE_PATH = "pictures/viking_room.png";

	int mipLevels = 1;

	// vector<VkDeviceMemory> vertexBufferMemorys; // TODO maybe move somewhere? To the "storage"?
	vector<VmaAllocation> vertexBufferAllocations; // TODO maybe move somewhere? To the "storage"?
	// vector<VkDeviceMemory> instanceBufferMemorys;
	vector<VmaAllocation> instanceBufferAllocations;
	vector<VmaAllocationInfo> instanceBufferAllocationInfos;
	// vector<VkBuffer> stagingBuffers; // TODO rename to stagingVertexBuffers
	// vector<VkDeviceMemory> stagingBufferMemorys;
	// vector<void*> stagingBufferMappedPointers;
	vector<VkBuffer> stagingInstanceBuffers;
	vector<VmaAllocation> stagingInstanceBufferAllocations;
	vector<VmaAllocationInfo> stagingInstanceBufferAllocationInfos;
	// vector<VkDeviceMemory> stagingInstanceBufferMemorys;
	// vector<void*> stagingInstanceBufferMappedPointers; // TODO rename somehow
	vector<VkImage> textureImages;
	// vector<VkDeviceMemory> textureImageMemorys;
	vector<VmaAllocation> textureAllocations;

	void createRenderPass();
	void createPipeline();

	void createTextureSampler();

	void createDescriptorSetLayout();
	void createDescriptorSetLayout2();

	void establishVertexBuffer(vector<Vertex> vertices, size_t id);
	void establishVertexBuffer2(vector<Vertex> vertices, size_t id);
	void establishInstanceBuffer(vector<Instance> instances, size_t id);
	void establishInstanceBuffer2(vector<Instance> instances, size_t id);
	// void establishTexture(int width, int height, void* pixels, VkImage& textureImage, VkImageView& textureImageView, VkDeviceMemory& textureImageMemory);
	void establishTextureVMA(int width, int height, void* pixels, VkImage& textureImage, VkImageView& textureImageView, VmaAllocation& textureAllocation);
};

#endif