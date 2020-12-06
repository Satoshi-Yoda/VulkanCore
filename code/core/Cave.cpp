#include "cave.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

using glm::vec2;
using glm::vec3;

using namespace std;

Cave::Cave() {
	aspects = CaveAspects::NONE;
}

Cave::~Cave() {
	// TODO free all allocated things
}

void Cave::setWorkingData(vector<Vertex> vertices, int width, int height, void* pixels) {
	this->vertices = vertices;
	this->width = width;
	this->height = height;
	this->pixels = pixels;
	aspects |= (CaveAspects::WORKING_VERTICES | CaveAspects::WORKING_INSTANCES | CaveAspects::WORKING_TEXTURE);
}

void Cave::setVulkanEntities(Rocks& rocks, Crater& crater) {
	this->rocks = &rocks;
	this->crater = &crater;
	aspects |= CaveAspects::VULKAN_ENTITIES;
}

bool Cave::has(CaveAspects aspects) {
	return (this->aspects & aspects) == aspects;
}

void Cave::establish(CaveAspects aspects) {
	if ((aspects & CaveAspects::STAGING_VERTICES)  != CaveAspects::NONE) establishStagingVertices();
	if ((aspects & CaveAspects::STAGING_INSTANCES) != CaveAspects::NONE) establishStagingInstances();
	if ((aspects & CaveAspects::STAGING_TEXTURE)   != CaveAspects::NONE) establishStagingTexture();
	if ((aspects & CaveAspects::LIVE_VERTICES)     != CaveAspects::NONE) establishLiveVertices();
	if ((aspects & CaveAspects::LIVE_INSTANCES)    != CaveAspects::NONE) establishLiveInstances();
	if ((aspects & CaveAspects::LIVE_TEXTURE)      != CaveAspects::NONE) establishLiveTexture();

	this->aspects |= aspects;
}

void Cave::free(CaveAspects aspects) {
	// TODO implement
	this->aspects &= aspects;
}

bool Cave::canBeDrawn() {
	return has(CaveAspects::LIVE_VERTICES | CaveAspects::LIVE_INSTANCES | CaveAspects::LIVE_TEXTURE);
}

void Cave::establishStagingVertices() {
	#ifdef use_validation
	if (!has(CaveAspects::WORKING_VERTICES | CaveAspects::VULKAN_ENTITIES)) {
		throw logic_error("In this cave there is no WORKING_VERTICES or no VULKAN_ENTITIES");
	}
	#endif

	vertexCount = static_cast<uint32_t>(vertices.size());
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

	rocks->createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingVertexBuffer, stagingVertexAllocation, stagingVertexInfo);
	memcpy(stagingVertexInfo.pMappedData, vertices.data(), static_cast<size_t>(bufferSize));

	aspects |= CaveAspects::STAGING_VERTICES;
}

void Cave::establishStagingInstances() {
	#ifdef use_validation
	if (!has(CaveAspects::WORKING_INSTANCES | CaveAspects::VULKAN_ENTITIES)) {
		throw logic_error("In this cave there is no WORKING_INSTANCES or no VULKAN_ENTITIES");
	}
	#endif

	instanceCount = static_cast<uint32_t>(instances.size());
	if (instanceCount == 0) return;
	VkDeviceSize bufferSize = sizeof(Instance) * instanceCount;

	rocks->createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingInstanceBuffer, stagingInstanceAllocation, stagingInstanceInfo);
	memcpy(stagingInstanceInfo.pMappedData, instances.data(), static_cast<size_t>(bufferSize));

	aspects |= CaveAspects::STAGING_INSTANCES;
}

void Cave::establishStagingTexture() {
	#ifdef use_validation
	if (!has(CaveAspects::WORKING_TEXTURE | CaveAspects::VULKAN_ENTITIES)) {
		throw logic_error("In this cave there is no WORKING_TEXTURE or no VULKAN_ENTITIES");
	}
	#endif

	VkDeviceSize imageSize = width * height * 4;

	rocks->createBufferVMA(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingTextureBuffer, stagingTextureAllocation, stagingTextureInfo);
	memcpy(stagingTextureInfo.pMappedData, pixels, static_cast<size_t>(imageSize));

	aspects |= CaveAspects::STAGING_TEXTURE;
}

void Cave::establishLiveVertices(VkCommandBuffer externalCommandBuffer) {
	#ifdef use_validation
	if (!has(CaveAspects::STAGING_VERTICES | CaveAspects::VULKAN_ENTITIES)) {
		throw logic_error("In this cave there is no STAGING_VERTICES or no VULKAN_ENTITIES");
	}
	#endif

	VkDeviceSize bufferSize = sizeof(Vertex) * vertexCount;

	rocks->createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingVertexBuffer, stagingVertexAllocation, stagingVertexInfo);
	memcpy(stagingVertexInfo.pMappedData, vertices.data(), static_cast<size_t>(bufferSize));

	rocks->createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, vertexBuffer, vertexAllocation, vertexInfo);

	bool useExternalCommandBuffer = (externalCommandBuffer != nullptr);
	VkCommandBuffer commandBuffer = useExternalCommandBuffer ? externalCommandBuffer : rocks->beginSingleTimeCommands();

	rocks->copyBufferToBuffer(stagingVertexBuffer, vertexBuffer, bufferSize, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, commandBuffer);

	if (useExternalCommandBuffer == false) {
		rocks->endSingleTimeCommands(commandBuffer);
	}

	aspects |= CaveAspects::LIVE_VERTICES;
}

void Cave::establishLiveInstances(VkCommandBuffer externalCommandBuffer) {
	#ifdef use_validation
	if (!has(CaveAspects::STAGING_INSTANCES | CaveAspects::VULKAN_ENTITIES)) {
		throw logic_error("In this cave there is no STAGING_INSTANCES or no VULKAN_ENTITIES");
	}
	#endif

	if (instanceCount == 0) return;
	VkDeviceSize bufferSize = sizeof(Instance) * instanceCount;

	rocks->createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, instanceBuffer, instanceAllocation, instanceInfo);

	bool useExternalCommandBuffer = (externalCommandBuffer != nullptr);
	VkCommandBuffer commandBuffer = useExternalCommandBuffer ? externalCommandBuffer : rocks->beginSingleTimeCommands();

	rocks->copyBufferToBuffer(stagingInstanceBuffer, instanceBuffer, bufferSize, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, commandBuffer);

	if (useExternalCommandBuffer == false) {
		rocks->endSingleTimeCommands(commandBuffer);
	}

	aspects |= CaveAspects::LIVE_INSTANCES;
}

void Cave::establishLiveTexture(VkCommandBuffer externalCommandBuffer) {
	#ifdef use_validation
	if (!has(CaveAspects::STAGING_TEXTURE | CaveAspects::VULKAN_ENTITIES)) {
		throw logic_error("In this cave there is no STAGING_TEXTURE or no VULKAN_ENTITIES");
	}
	#endif

	auto preferred8bitFormat = crater->USE_GAMMA_CORRECT ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
	uint32_t mipLevels = 1;
	// mipLevels = static_cast<uint32_t>(floor(log2(max(width, height)))) + 1;

	rocks->createImageVMA(static_cast<uint32_t>(width), static_cast<uint32_t>(height), mipLevels, VK_SAMPLE_COUNT_1_BIT, preferred8bitFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_MEMORY_USAGE_GPU_ONLY,
		textureImage, textureAllocation);

	bool useExternalCommandBuffer = (externalCommandBuffer != nullptr);
	VkCommandBuffer commandBuffer = useExternalCommandBuffer ? externalCommandBuffer : rocks->beginSingleTimeCommands();

	rocks->transitionImageLayout(textureImage, preferred8bitFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels, commandBuffer); // TODO check why in tutorial no mipLevels here
	rocks->copyBufferToImage(stagingTextureBuffer, textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height), commandBuffer);
	rocks->transitionImageLayout(textureImage, preferred8bitFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels, commandBuffer);
	// generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, width, height, mipLevels);

	if (useExternalCommandBuffer == false) {
		rocks->endSingleTimeCommands(commandBuffer);
	}

	textureView = rocks->createImageView(textureImage, preferred8bitFormat, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

	aspects |= CaveAspects::LIVE_TEXTURE;
}
