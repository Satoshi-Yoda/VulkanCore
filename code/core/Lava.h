#ifndef LAVA_H
#define LAVA_H

#include <mutex>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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

	size_t addObject(vector<Vertex> vertices, vector<Instance> instances, int width, int height, void* pixels);
	size_t texturesCount();
	void updateVertexBuffer(size_t id, vector<Vertex> vertices);
	void updateInstanceBuffer(size_t id, vector<Instance> instances);
	void updateInstances(size_t id, vector<Instance> instances, vector<size_t> indexes);

private:
	Ash& ash;
	Mountain& mountain;
	Rocks& rocks;
	Crater& crater;

	const string TEXTURE_PATH = "pictures/viking_room.png";

	int mipLevels = 1;

	vector<VkDeviceMemory> vertexBufferMemorys; // TODO maybe move somewhere? To the "storage"?
	vector<VkDeviceMemory> instanceBufferMemorys;
	vector<VkBuffer> stagingBuffers; // TODO rename to stagingVertexBuffers
	vector<VkDeviceMemory> stagingBufferMemorys;
	vector<void*> stagingBufferMappedPointers;
	vector<VkBuffer> stagingInstanceBuffers;
	vector<VkDeviceMemory> stagingInstanceBufferMemorys;
	vector<void*> stagingInstanceBufferMappedPointers; // TODO rename somehow
	vector<VkImage> textureImages;
	vector<VkDeviceMemory> textureImageMemorys;

	mutex establishMutex;

	void createRenderPass();
	void createPipeline();

	void createTextureSampler();

	void createDescriptorSetLayout();
	void createDescriptorSetLayout2();

	void establishVertexBuffer(vector<Vertex> vertices, size_t id);
	void establishInstanceBuffer(vector<Instance> instances, size_t id);
	void establishTexture(int width, int height, void* pixels, VkImage& textureImage, VkImageView& textureImageView, VkDeviceMemory& textureImageMemory);
};

#endif