#ifndef LAVA_H
#define LAVA_H

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "Mountain.h"
#include "Rocks.h"
#include "Crater.h"

using std::vector;
using std::array;
using glm::vec2;
using glm::vec3;

// TODO maybe move Vertex into separate file
struct Vertex {
	vec3 pos;
	vec3 color;
	vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription();
	static array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
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
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout descriptorSetLayout2;

	vector<VkImageView> textureImageViews;
	VkSampler textureSampler;

	void addObject(vector<Vertex> vertices, int width, int height, void* pixels);

private:
	Ash& ash;
	Mountain& mountain;
	Rocks& rocks;
	Crater& crater;

	const string TEXTURE_PATH = "pictures/viking_room.png";

	int mipLevels = 1;

	vector<VkDeviceMemory> vertexBufferMemorys; // TODO maybe move somewhere? To the "storage"?
	vector<VkImage> textureImages;
	vector<VkDeviceMemory> textureImageMemorys;

	void createRenderPass();
	void createPipeline();

	void createTextureSampler();

	void createDescriptorSetLayout();
	void createDescriptorSetLayout2();

	void establishVertexBuffer(vector<Vertex> vertices, VkBuffer& vertexBufferL, uint32_t& vertexBufferSizeL, VkDeviceMemory& vertexBufferMemoryL);
	void establishTexture(int width, int height, void* pixels, VkImage& textureImageL, VkImageView& textureImageViewL, VkDeviceMemory& textureImageMemoryL);
};

#endif