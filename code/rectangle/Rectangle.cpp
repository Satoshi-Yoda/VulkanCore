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
	if (this->aspects.has(RectangleAspect::STAGING_TEXTURE))   freeStagingTexture();
	if (this->aspects.has(RectangleAspect::LIVE_VERTICES))     freeLiveVertices();
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
	aspects.raise(RectangleAspect::WORKING_VERTICES, RectangleAspect::WORKING_TEXTURE);
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
	if (aspect == RectangleAspect::STAGING_TEXTURE)   establishStagingTexture();
	if (aspect == RectangleAspect::LIVE_VERTICES)     establishLiveVertices();
	if (aspect == RectangleAspect::LIVE_TEXTURE)      establishLiveTexture();
}

void Rectangle::establish(VkCommandBuffer cb, RectangleAspect aspect) {
	if (aspect == RectangleAspect::LIVE_VERTICES)  establishLiveVertices(cb);
	if (aspect == RectangleAspect::LIVE_TEXTURE)   establishLiveTexture(cb);
}

void Rectangle::refresh(RectangleAspect aspect) { }

void Rectangle::createDescriptorSet() {
	assert(mountain != nullptr);

	VkDescriptorSetAllocateInfo allocInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = mountain->descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &lava->rectangleLayout.descriptorSetLayout;

	vkAllocateDescriptorSets(mountain->device, &allocInfo, &descriptorSet) >> ash("Failed to allocate descriptor set!");

	VkDescriptorImageInfo imageInfo {};
	imageInfo.sampler = lava->rectangleLayout.textureSampler;
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

	// can be used to update several descriptor sets at once
	vkUpdateDescriptorSets(mountain->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Rectangle::free(RectangleAspect aspect) {
	if (aspect == RectangleAspect::STAGING_VERTICES  && this->aspects.has(aspect)) freeStagingVertices();
	if (aspect == RectangleAspect::STAGING_TEXTURE   && this->aspects.has(aspect)) freeStagingTexture();
	if (aspect == RectangleAspect::LIVE_VERTICES     && this->aspects.has(aspect)) freeLiveVertices();
	if (aspect == RectangleAspect::LIVE_TEXTURE      && this->aspects.has(aspect)) freeLiveTexture();
}

void Rectangle::establishStagingVertices() {
	#ifdef use_validation
	aspects.has(RectangleAspect::WORKING_VERTICES, RectangleAspect::VULKAN_ENTITIES) >> ash("In this rectangle there is no WORKING_VERTICES or no VULKAN_ENTITIES");
	#endif

	stagingVertexCount = static_cast<uint32_t>(vertices.size());
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

	rocks->createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingVertexBuffer, stagingVertexAllocation, stagingVertexInfo);
	memcpy(stagingVertexInfo.pMappedData, vertices.data(), static_cast<size_t>(bufferSize));

	aspects.raise(RectangleAspect::STAGING_VERTICES);
}

void Rectangle::establishStagingTexture() {
	#ifdef use_validation
	aspects.has(RectangleAspect::WORKING_TEXTURE, RectangleAspect::VULKAN_ENTITIES) >> ash("In this rectangle there is no WORKING_TEXTURE or no VULKAN_ENTITIES");
	#endif

	VkDeviceSize imageSize = width * height * 4;

	rocks->createBufferVMA(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingTextureBuffer, stagingTextureAllocation, stagingTextureInfo);
	memcpy(stagingTextureInfo.pMappedData, pixels, static_cast<size_t>(imageSize));

	aspects.raise(RectangleAspect::STAGING_TEXTURE);
}

void Rectangle::establishLiveVertices(VkCommandBuffer externalCommandBuffer) {
	#ifdef use_validation
	aspects.has(RectangleAspect::STAGING_VERTICES, RectangleAspect::VULKAN_ENTITIES) >> ash("In this rectangle there is no STAGING_VERTICES or no VULKAN_ENTITIES");
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

void Rectangle::establishLiveTexture(VkCommandBuffer externalCommandBuffer) {
	#ifdef use_validation
	aspects.has(RectangleAspect::STAGING_TEXTURE, RectangleAspect::VULKAN_ENTITIES) >> ash("In this rectangle there is no STAGING_TEXTURE or no VULKAN_ENTITIES");
	#endif

	auto preferred8bitFormat = crater->USE_GAMMA_CORRECT ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM; // VK_FORMAT_R32_SFLOAT 
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

void Rectangle::freeLiveTexture() {
	vkQueueWaitIdle(mountain->queue);
	vkDestroyImageView(mountain->device, textureView, nullptr);
	vmaDestroyImage(mountain->allocator, textureImage, textureAllocation);
	aspects.drop(RectangleAspect::LIVE_TEXTURE);
}
