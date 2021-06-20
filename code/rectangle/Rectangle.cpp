#include "Rectangle.h"

#include <cassert>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../utils/Loader.h"

using glm::vec2;
using glm::vec3;

using namespace std;

Rectangle::Rectangle(Ash& ash) : ash(ash) { }

Rectangle::~Rectangle() {
	freeTexture(pixels);
	if (this->aspects.has(RectangleAspect::STAGING_VERTICES))  freeStagingVertices();
	if (this->aspects.has(RectangleAspect::STAGING_INSTANCES)) freeStagingInstances();
	if (this->aspects.has(RectangleAspect::STAGING_TEXTURE))   freeStagingTexture();
	if (this->aspects.has(RectangleAspect::LIVE_VERTICES))     freeLiveVertices();
	if (this->aspects.has(RectangleAspect::LIVE_INSTANCES))    freeLiveInstances();
	if (this->aspects.has(RectangleAspect::LIVE_TEXTURE))      freeLiveTexture();
}

void Rectangle::setName(string name) {
	this->name = name;
}

void Rectangle::setWorkingData(vector<Vertex> vertices, int width, int height, void* pixels) {
	this->vertices = vertices;
	this->width = width;
	this->height = height;
	this->pixels = pixels;
	aspects.raise(RectangleAspect::WORKING_VERTICES, RectangleAspect::WORKING_INSTANCES, RectangleAspect::WORKING_TEXTURE);
}

void Rectangle::setVulkanEntities(Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) {
	this->mountain = &mountain;
	this->rocks = &rocks;
	this->crater = &crater;
	this->lava = &lava;
	aspects.raise(RectangleAspect::VULKAN_ENTITIES);
}

void Rectangle::establish(RectangleAspect aspect) {
	if (aspect == RectangleAspect::STAGING_VERTICES)  establishStagingVertices();
	if (aspect == RectangleAspect::STAGING_INSTANCES) establishStagingInstances();
	if (aspect == RectangleAspect::STAGING_TEXTURE)   establishStagingTexture();
	if (aspect == RectangleAspect::LIVE_VERTICES)     establishLiveVertices();
	if (aspect == RectangleAspect::LIVE_INSTANCES)    establishLiveInstances();
	if (aspect == RectangleAspect::LIVE_TEXTURE)      establishLiveTexture();
}

void Rectangle::establish(VkCommandBuffer cb, RectangleAspect aspect) {
	if (aspect == RectangleAspect::LIVE_VERTICES)  establishLiveVertices(cb);
	if (aspect == RectangleAspect::LIVE_INSTANCES) establishLiveInstances(cb);
	if (aspect == RectangleAspect::LIVE_TEXTURE)   establishLiveTexture(cb);
}

void Rectangle::refresh(RectangleAspect aspect) {
	if (aspect == RectangleAspect::STAGING_INSTANCES) {
		if (instances.size() != stagingInstanceCount) {
			if (this->aspects.has(RectangleAspect::STAGING_INSTANCES)) {
				freeStagingInstances();
			}
			establishStagingInstances();
		} else {
			refreshStagingInstances();
		}
	}

	if (aspect == RectangleAspect::LIVE_INSTANCES) {
		if (stagingInstanceCount != instanceCount) {
			if (this->aspects.has(RectangleAspect::LIVE_INSTANCES)) {
				freeLiveInstances();
			}
			establishLiveInstances();
		} else {
			refreshLiveInstances();
		}
	}
}

void Rectangle::updateInstances(vector<size_t> indexes) {
	assert(indexes.size() > 0);

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

void Rectangle::createDescriptorSet() {
	assert(mountain != nullptr);

	VkDescriptorSetAllocateInfo allocInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = mountain->descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &lava->batchLayout.descriptorSetLayout;

	vkAllocateDescriptorSets(mountain->device, &allocInfo, &descriptorSet) >> ash("Failed to allocate descriptor set!");

	VkDescriptorImageInfo imageInfo {};
	imageInfo.sampler = lava->batchLayout.textureSampler;
	imageInfo.imageView = textureView;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorBufferInfo uniformInfo {};
	uniformInfo.buffer = lava->uniformBuffer;
	uniformInfo.offset = 0;
	uniformInfo.range = sizeof(UniformBufferObject);

	VkWriteDescriptorSet imageWrite { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	imageWrite.dstSet = descriptorSet;
	imageWrite.dstBinding = 0;
	imageWrite.dstArrayElement = 0;
	imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	imageWrite.descriptorCount = 1;
	imageWrite.pImageInfo = &imageInfo;

	VkWriteDescriptorSet uniformWrite { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	uniformWrite.dstSet = descriptorSet;
	uniformWrite.dstBinding = 1;
	uniformWrite.dstArrayElement = 0;
	uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformWrite.descriptorCount = 1;
	uniformWrite.pBufferInfo = &uniformInfo;

	array<VkWriteDescriptorSet, 2> descriptorWrites { imageWrite, uniformWrite };

	vkUpdateDescriptorSets(mountain->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Rectangle::free(RectangleAspect aspect) {
	if (aspect == RectangleAspect::STAGING_VERTICES  && this->aspects.has(aspect)) freeStagingVertices();
	if (aspect == RectangleAspect::STAGING_INSTANCES && this->aspects.has(aspect)) freeStagingInstances();
	if (aspect == RectangleAspect::STAGING_TEXTURE   && this->aspects.has(aspect)) freeStagingTexture();
	if (aspect == RectangleAspect::LIVE_VERTICES     && this->aspects.has(aspect)) freeLiveVertices();
	if (aspect == RectangleAspect::LIVE_INSTANCES    && this->aspects.has(aspect)) freeLiveInstances();
	if (aspect == RectangleAspect::LIVE_TEXTURE      && this->aspects.has(aspect)) freeLiveTexture();
}

void Rectangle::establishStagingVertices() {
	#ifdef use_validation
	aspects.has(RectangleAspect::WORKING_VERTICES, RectangleAspect::VULKAN_ENTITIES) >> ash("In this cave there is no WORKING_VERTICES or no VULKAN_ENTITIES");
	#endif

	stagingVertexCount = static_cast<uint32_t>(vertices.size());
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

	rocks->createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingVertexBuffer, stagingVertexAllocation, stagingVertexInfo);
	memcpy(stagingVertexInfo.pMappedData, vertices.data(), static_cast<size_t>(bufferSize));

	aspects.raise(RectangleAspect::STAGING_VERTICES);
}

void Rectangle::establishStagingInstances() {
	#ifdef use_validation
	aspects.has(RectangleAspect::WORKING_INSTANCES, RectangleAspect::VULKAN_ENTITIES) >> ash("In this cave there is no WORKING_INSTANCES or no VULKAN_ENTITIES");
	#endif

	stagingInstanceCount = static_cast<uint32_t>(instances.size());
	if (stagingInstanceCount == 0) return;
	VkDeviceSize bufferSize = sizeof(Instance) * stagingInstanceCount;

	rocks->createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingInstanceBuffer, stagingInstanceAllocation, stagingInstanceInfo);
	memcpy(stagingInstanceInfo.pMappedData, instances.data(), static_cast<size_t>(bufferSize));

	aspects.raise(RectangleAspect::STAGING_INSTANCES);
}

void Rectangle::refreshStagingInstances() {
	#ifdef use_validation
	aspects.has(RectangleAspect::WORKING_INSTANCES, RectangleAspect::VULKAN_ENTITIES) >> ash("In this cave there is no WORKING_INSTANCES or no VULKAN_ENTITIES");
	(stagingInstanceCount == static_cast<uint32_t>(instances.size()))       >> ash("copyStagingInstances() called when stagingInstanceCount != instances.size()");
	#endif

	if (stagingInstanceCount == 0 || instances.size() == 0) return;
	VkDeviceSize bufferSize = sizeof(Instance) * min(static_cast<size_t>(stagingInstanceCount), instances.size());

	memcpy(stagingInstanceInfo.pMappedData, instances.data(), static_cast<size_t>(bufferSize));
}

void Rectangle::establishStagingTexture() {
	#ifdef use_validation
	aspects.has(RectangleAspect::WORKING_TEXTURE, RectangleAspect::VULKAN_ENTITIES) >> ash("In this cave there is no WORKING_TEXTURE or no VULKAN_ENTITIES");
	#endif

	VkDeviceSize imageSize = width * height * 4;

	rocks->createBufferVMA(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingTextureBuffer, stagingTextureAllocation, stagingTextureInfo);
	memcpy(stagingTextureInfo.pMappedData, pixels, static_cast<size_t>(imageSize));

	aspects.raise(RectangleAspect::STAGING_TEXTURE);
}

void Rectangle::establishLiveVertices(VkCommandBuffer externalCommandBuffer) {
	#ifdef use_validation
	aspects.has(RectangleAspect::STAGING_VERTICES, RectangleAspect::VULKAN_ENTITIES) >> ash("In this cave there is no STAGING_VERTICES or no VULKAN_ENTITIES");
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

	aspects.raise(RectangleAspect::LIVE_VERTICES);
}

void Rectangle::establishLiveInstances(VkCommandBuffer externalCommandBuffer) {
	if (stagingInstanceCount == 0) return;

	#ifdef use_validation
	aspects.has(RectangleAspect::STAGING_INSTANCES, RectangleAspect::VULKAN_ENTITIES) >> ash("In this cave there is no STAGING_INSTANCES or no VULKAN_ENTITIES");
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

	aspects.raise(RectangleAspect::LIVE_INSTANCES);
}

void Rectangle::refreshLiveInstances(VkCommandBuffer externalCommandBuffer) {
	if (instanceCount == 0) return;

	#ifdef use_validation
	aspects.has(RectangleAspect::STAGING_INSTANCES, RectangleAspect::VULKAN_ENTITIES) >> ash("In this cave there is no STAGING_INSTANCES or no VULKAN_ENTITIES");
	#endif

	VkDeviceSize bufferSize = sizeof(Instance) * instanceCount;

	bool useExternalCommandBuffer = (externalCommandBuffer != nullptr);
	VkCommandBuffer commandBuffer = useExternalCommandBuffer ? externalCommandBuffer : rocks->beginSingleTimeCommands();

	rocks->copyBufferToBuffer(stagingInstanceBuffer, instanceBuffer, bufferSize, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, commandBuffer);

	if (useExternalCommandBuffer == false) {
		rocks->endSingleTimeCommands(commandBuffer);
	}
}

void Rectangle::establishLiveTexture(VkCommandBuffer externalCommandBuffer) {
	#ifdef use_validation
	aspects.has(RectangleAspect::STAGING_TEXTURE, RectangleAspect::VULKAN_ENTITIES) >> ash("In this cave there is no STAGING_TEXTURE or no VULKAN_ENTITIES");
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

	aspects.raise(RectangleAspect::LIVE_TEXTURE);
}

void Rectangle::freeStagingVertices() {
	// TODO can be optimized by calling this later in future frames, without wait, or something
	vkQueueWaitIdle(mountain->queue); // TODO do I need this for staging resources?
	vmaDestroyBuffer(mountain->allocator, stagingVertexBuffer, stagingVertexAllocation);
	aspects.drop(RectangleAspect::STAGING_VERTICES);
}

void Rectangle::freeStagingInstances() {
	vkQueueWaitIdle(mountain->queue);
	vmaDestroyBuffer(mountain->allocator, stagingInstanceBuffer, stagingInstanceAllocation);
	aspects.drop(RectangleAspect::STAGING_INSTANCES);
}

void Rectangle::freeStagingTexture() {
	vkQueueWaitIdle(mountain->queue);
	vmaDestroyBuffer(mountain->allocator, stagingTextureBuffer, stagingTextureAllocation);
	aspects.drop(RectangleAspect::STAGING_TEXTURE);
}

void Rectangle::freeLiveVertices() {
	vkQueueWaitIdle(mountain->queue);
	vmaDestroyBuffer(mountain->allocator, vertexBuffer, vertexAllocation);
	aspects.drop(RectangleAspect::LIVE_VERTICES);
}

void Rectangle::freeLiveInstances() {
	vkQueueWaitIdle(mountain->queue);
	vmaDestroyBuffer(mountain->allocator, instanceBuffer, instanceAllocation);
	aspects.drop(RectangleAspect::LIVE_INSTANCES);
}

void Rectangle::freeLiveTexture() {
	vkQueueWaitIdle(mountain->queue);
	vkDestroyImageView(mountain->device, textureView, nullptr);
	vmaDestroyImage(mountain->allocator, textureImage, textureAllocation);
	aspects.drop(RectangleAspect::LIVE_TEXTURE);
}
