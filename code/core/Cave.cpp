#include "cave.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../utils/Loader.h"

using glm::vec2;
using glm::vec3;

using namespace std;

Cave::Cave() { }

Cave::~Cave() {
	freeTexture(pixels);
	if (this->aspects.has(CaveAspect::STAGING_VERTICES))  freeStagingVertices();
	if (this->aspects.has(CaveAspect::STAGING_INSTANCES)) freeStagingInstances();
	if (this->aspects.has(CaveAspect::STAGING_TEXTURE))   freeStagingTexture();
	if (this->aspects.has(CaveAspect::LIVE_VERTICES))     freeLiveVertices();
	if (this->aspects.has(CaveAspect::LIVE_INSTANCES))    freeLiveInstances();
	if (this->aspects.has(CaveAspect::LIVE_TEXTURE))      freeLiveTexture();
}

void Cave::setName(string name) {
	this->name = name;
}

void Cave::setWorkingData(vector<Vertex> vertices, int width, int height, void* pixels) {
	this->vertices = vertices;
	this->width = width;
	this->height = height;
	this->pixels = pixels;
	aspects.raise(CaveAspect::WORKING_VERTICES, CaveAspect::WORKING_INSTANCES, CaveAspect::WORKING_TEXTURE);
}

void Cave::setVulkanEntities(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater) {
	this->ash = &ash;
	this->mountain = &mountain;
	this->rocks = &rocks;
	this->crater = &crater;
	aspects.raise(CaveAspect::VULKAN_ENTITIES);
}

void Cave::establish(CaveAspect aspect) {
	if (aspect == CaveAspect::STAGING_VERTICES)  establishStagingVertices();
	if (aspect == CaveAspect::STAGING_INSTANCES) establishStagingInstances();
	if (aspect == CaveAspect::STAGING_TEXTURE)   establishStagingTexture();
	if (aspect == CaveAspect::LIVE_VERTICES)     establishLiveVertices();
	if (aspect == CaveAspect::LIVE_INSTANCES)    establishLiveInstances();
	if (aspect == CaveAspect::LIVE_TEXTURE)      establishLiveTexture();
}

void Cave::mount(VkCommandBuffer commandBuffer, CaveAspect aspect) {
	if (aspect == CaveAspect::LIVE_VERTICES)  establishLiveVertices(commandBuffer);
	if (aspect == CaveAspect::LIVE_INSTANCES) establishLiveInstances(commandBuffer);
	if (aspect == CaveAspect::LIVE_TEXTURE)   establishLiveTexture(commandBuffer);
}

void Cave::refresh(CaveAspect aspect) {
	if (aspect == CaveAspect::STAGING_INSTANCES) {
		if (instances.size() != stagingInstanceCount) {
			if (this->aspects.has(CaveAspect::STAGING_INSTANCES)) {
				freeStagingInstances();
			}
			establishStagingInstances();
		} else {
			refreshStagingInstances();
		}
	}

	if (aspect == CaveAspect::LIVE_INSTANCES) {
		if (stagingInstanceCount != instanceCount) {
			if (this->aspects.has(CaveAspect::LIVE_INSTANCES)) {
				freeLiveInstances();
			}
			establishLiveInstances();
		} else {
			refreshLiveInstances();
		}
	}
}

void Cave::updateInstances(vector<size_t> indexes) {
	Instance* stagingVector = reinterpret_cast<Instance*>(stagingInstanceInfo.pMappedData);
	for (auto& index : indexes) {
		stagingVector[index] = instances[index];
	}

	vector<VkBufferCopy> regions;
	regions.resize(indexes.size());

	for (size_t i = 0; i < indexes.size(); i++) {
		VkDeviceSize offset = indexes[i] * sizeof(Instance);
		regions[i].srcOffset = offset;
		regions[i].dstOffset = offset;
		regions[i].size = sizeof(Instance);
	}

	VkBuffer& buffer = instanceBuffer;
	VkBuffer& stagingBuffer = stagingInstanceBuffer;

	rocks->copyBufferToBuffer(stagingBuffer, buffer, regions, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
}

void Cave::free(CaveAspect aspect) {
	if (aspect == CaveAspect::STAGING_VERTICES  && this->aspects.has(aspect)) freeStagingVertices();
	if (aspect == CaveAspect::STAGING_INSTANCES && this->aspects.has(aspect)) freeStagingInstances();
	if (aspect == CaveAspect::STAGING_TEXTURE   && this->aspects.has(aspect)) freeStagingTexture();
	if (aspect == CaveAspect::LIVE_VERTICES     && this->aspects.has(aspect)) freeLiveVertices();
	if (aspect == CaveAspect::LIVE_INSTANCES    && this->aspects.has(aspect)) freeLiveInstances();
	if (aspect == CaveAspect::LIVE_TEXTURE      && this->aspects.has(aspect)) freeLiveTexture();
}

void Cave::establishStagingVertices() {
	#ifdef use_validation
	// false >> (*ash)("In this cave there is no WORKING_VERTICES or no VULKAN_ENTITIES");
	aspects.has(CaveAspect::WORKING_VERTICES, CaveAspect::VULKAN_ENTITIES) >> (*ash)("In this cave there is no WORKING_VERTICES or no VULKAN_ENTITIES");
	// aspects.has(CaveAspect::WORKING_VERTICES) >> (*ash)("In this cave there is no WORKING_VERTICES");
	// aspects.has(CaveAspect::VULKAN_ENTITIES) >> (*ash)("In this cave there is no VULKAN_ENTITIES");
	#endif

	stagingVertexCount = static_cast<uint32_t>(vertices.size());
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

	rocks->createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingVertexBuffer, stagingVertexAllocation, stagingVertexInfo);
	memcpy(stagingVertexInfo.pMappedData, vertices.data(), static_cast<size_t>(bufferSize));

	aspects.raise(CaveAspect::STAGING_VERTICES);
}

void Cave::establishStagingInstances() {
	#ifdef use_validation
	aspects.has(CaveAspect::WORKING_INSTANCES, CaveAspect::VULKAN_ENTITIES) >> (*ash)("In this cave there is no WORKING_INSTANCES or no VULKAN_ENTITIES");
	#endif

	stagingInstanceCount = static_cast<uint32_t>(instances.size());
	if (stagingInstanceCount == 0) return;
	VkDeviceSize bufferSize = sizeof(Instance) * stagingInstanceCount;

	rocks->createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingInstanceBuffer, stagingInstanceAllocation, stagingInstanceInfo);
	memcpy(stagingInstanceInfo.pMappedData, instances.data(), static_cast<size_t>(bufferSize));

	aspects.raise(CaveAspect::STAGING_INSTANCES);
}

void Cave::refreshStagingInstances() {
	#ifdef use_validation
	aspects.has(CaveAspect::WORKING_INSTANCES, CaveAspect::VULKAN_ENTITIES) >> (*ash)("In this cave there is no WORKING_INSTANCES or no VULKAN_ENTITIES");
	(stagingInstanceCount == static_cast<uint32_t>(instances.size()))         >> (*ash)("copyStagingInstances() called when stagingInstanceCount != instances.size()");
	#endif

	if (stagingInstanceCount == 0 || instances.size() == 0) return;
	VkDeviceSize bufferSize = sizeof(Instance) * min(static_cast<size_t>(stagingInstanceCount), instances.size());

	memcpy(stagingInstanceInfo.pMappedData, instances.data(), static_cast<size_t>(bufferSize));
}

void Cave::establishStagingTexture() {
	#ifdef use_validation
	aspects.has(CaveAspect::WORKING_TEXTURE, CaveAspect::VULKAN_ENTITIES) >> (*ash)("In this cave there is no WORKING_TEXTURE or no VULKAN_ENTITIES");
	#endif

	VkDeviceSize imageSize = width * height * 4;

	rocks->createBufferVMA(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingTextureBuffer, stagingTextureAllocation, stagingTextureInfo);
	memcpy(stagingTextureInfo.pMappedData, pixels, static_cast<size_t>(imageSize));

	aspects.raise(CaveAspect::STAGING_TEXTURE);
}

void Cave::establishLiveVertices(VkCommandBuffer externalCommandBuffer) {
	#ifdef use_validation
	aspects.has(CaveAspect::STAGING_VERTICES, CaveAspect::VULKAN_ENTITIES) >> (*ash)("In this cave there is no STAGING_VERTICES or no VULKAN_ENTITIES");
	#endif

	VkDeviceSize bufferSize = sizeof(Vertex) * stagingVertexCount;
	vertexCount = stagingVertexCount;

	rocks->createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, vertexBuffer, vertexAllocation, vertexInfo);

	bool useExternalCommandBuffer = (externalCommandBuffer != nullptr);
	VkCommandBuffer commandBuffer = useExternalCommandBuffer ? externalCommandBuffer : rocks->beginSingleTimeCommands();

	rocks->copyBufferToBuffer(stagingVertexBuffer, vertexBuffer, bufferSize, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, commandBuffer);

	if (useExternalCommandBuffer == false) {
		rocks->endSingleTimeCommands(commandBuffer);
	}

	aspects.raise(CaveAspect::LIVE_VERTICES);
}

void Cave::establishLiveInstances(VkCommandBuffer externalCommandBuffer) {
	if (stagingInstanceCount == 0) return;

	#ifdef use_validation
	aspects.has(CaveAspect::STAGING_INSTANCES, CaveAspect::VULKAN_ENTITIES) >> (*ash)("In this cave there is no STAGING_INSTANCES or no VULKAN_ENTITIES");
	#endif

	VkDeviceSize bufferSize = sizeof(Instance) * stagingInstanceCount;
	instanceCount = stagingInstanceCount;

	rocks->createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, instanceBuffer, instanceAllocation, instanceInfo);

	bool useExternalCommandBuffer = (externalCommandBuffer != nullptr);
	VkCommandBuffer commandBuffer = useExternalCommandBuffer ? externalCommandBuffer : rocks->beginSingleTimeCommands();

	rocks->copyBufferToBuffer(stagingInstanceBuffer, instanceBuffer, bufferSize, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, commandBuffer);

	if (useExternalCommandBuffer == false) {
		rocks->endSingleTimeCommands(commandBuffer);
	}

	aspects.raise(CaveAspect::LIVE_INSTANCES);
}

void Cave::refreshLiveInstances(VkCommandBuffer externalCommandBuffer) {
	if (instanceCount == 0) return;

	#ifdef use_validation
	aspects.has(CaveAspect::STAGING_INSTANCES, CaveAspect::VULKAN_ENTITIES) >> (*ash)("In this cave there is no STAGING_INSTANCES or no VULKAN_ENTITIES");
	#endif

	VkDeviceSize bufferSize = sizeof(Instance) * instanceCount;

	bool useExternalCommandBuffer = (externalCommandBuffer != nullptr);
	VkCommandBuffer commandBuffer = useExternalCommandBuffer ? externalCommandBuffer : rocks->beginSingleTimeCommands();

	rocks->copyBufferToBuffer(stagingInstanceBuffer, instanceBuffer, bufferSize, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, commandBuffer);

	if (useExternalCommandBuffer == false) {
		rocks->endSingleTimeCommands(commandBuffer);
	}
}

void Cave::establishLiveTexture(VkCommandBuffer externalCommandBuffer) {
	#ifdef use_validation
	aspects.has(CaveAspect::STAGING_TEXTURE, CaveAspect::VULKAN_ENTITIES) >> (*ash)("In this cave there is no STAGING_TEXTURE or no VULKAN_ENTITIES");
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

	aspects.raise(CaveAspect::LIVE_TEXTURE);
}

void Cave::freeStagingVertices() {
	// TODO can be optimized by calling this later in future frames, without wait, or something
	vkQueueWaitIdle(mountain->queue);
	vmaDestroyBuffer(mountain->allocator, stagingVertexBuffer, stagingVertexAllocation);
	aspects.drop(CaveAspect::STAGING_VERTICES);
}

void Cave::freeStagingInstances() {
	vkQueueWaitIdle(mountain->queue);
	vmaDestroyBuffer(mountain->allocator, stagingInstanceBuffer, stagingInstanceAllocation);
	aspects.drop(CaveAspect::STAGING_INSTANCES);
}

void Cave::freeStagingTexture() {
	vkQueueWaitIdle(mountain->queue);
	vmaDestroyBuffer(mountain->allocator, stagingTextureBuffer, stagingTextureAllocation);
	aspects.drop(CaveAspect::STAGING_TEXTURE);
}

void Cave::freeLiveVertices() {
	vkQueueWaitIdle(mountain->queue);
	vmaDestroyBuffer(mountain->allocator, vertexBuffer, vertexAllocation);
	aspects.drop(CaveAspect::LIVE_VERTICES);
}

void Cave::freeLiveInstances() {
	vkQueueWaitIdle(mountain->queue);
	vmaDestroyBuffer(mountain->allocator, instanceBuffer, instanceAllocation);
	aspects.drop(CaveAspect::LIVE_INSTANCES);
}

void Cave::freeLiveTexture() {
	vkQueueWaitIdle(mountain->queue);
	vkDestroyImageView(mountain->device, textureView, nullptr);
	vmaDestroyImage(mountain->allocator, textureImage, textureAllocation);
	aspects.drop(CaveAspect::LIVE_TEXTURE);
}
