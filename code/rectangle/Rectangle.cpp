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
	if (this->aspects.has(RectangleAspect::STAGING_VERTICES)) freeStagingVertices();
	if (this->aspects.has(RectangleAspect::LIVE_VERTICES))    freeLiveVertices();
}

void Rectangle::setName(string name) {
	this->name = name;
}

void Rectangle::setWorkingData(vector<Vertex> vertices) {
	this->vertices = vertices;
	aspects.raise(RectangleAspect::WORKING_VERTICES);
}

void Rectangle::setVulkanEntities(Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) {
	this->mountain = &mountain;
	this->rocks = &rocks;
	this->crater = &crater;
	this->lava = &lava;
	aspects.raise(RectangleAspect::VULKAN_ENTITIES);
}

void Rectangle::establish(RectangleAspect aspect) {
	if (aspect == RectangleAspect::STAGING_VERTICES) establishStagingVertices();
	if (aspect == RectangleAspect::LIVE_VERTICES)    establishLiveVertices();
}

void Rectangle::establish(VkCommandBuffer cb, RectangleAspect aspect) {
	if (aspect == RectangleAspect::LIVE_VERTICES) establishLiveVertices(cb);
}

void Rectangle::refresh(RectangleAspect aspect) { }

void Rectangle::createDescriptorSet() {
	assert(mountain != nullptr);

	VkDescriptorSetAllocateInfo allocInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = mountain->descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &lava->rectangleLayout.descriptorSetLayout;

	vkAllocateDescriptorSets(mountain->device, &allocInfo, &descriptorSet) >> ash("Failed to allocate descriptor set!");

	VkDescriptorBufferInfo uniformInfo {};
	uniformInfo.buffer = lava->uniformBuffer;
	uniformInfo.offset = 0;
	uniformInfo.range = sizeof(UniformBufferObject);

	VkWriteDescriptorSet uniformWrite { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	uniformWrite.dstSet = descriptorSet;
	uniformWrite.dstBinding = 1;
	uniformWrite.dstArrayElement = 0;
	uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformWrite.descriptorCount = 1;
	uniformWrite.pBufferInfo = &uniformInfo;

	array<VkWriteDescriptorSet, 1> descriptorWrites { uniformWrite };

	// can be used to update several descriptor sets at once
	vkUpdateDescriptorSets(mountain->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Rectangle::free(RectangleAspect aspect) {
	if (aspect == RectangleAspect::STAGING_VERTICES && this->aspects.has(aspect)) freeStagingVertices();
	if (aspect == RectangleAspect::LIVE_VERTICES    && this->aspects.has(aspect)) freeLiveVertices();
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

void Rectangle::freeStagingVertices() {
	// TODO can be optimized by calling this later in future frames, without wait, or something
	vkQueueWaitIdle(mountain->queue); // TODO do I need this for staging resources?
	vmaDestroyBuffer(mountain->allocator, stagingVertexBuffer, stagingVertexAllocation);
	aspects.drop(RectangleAspect::STAGING_VERTICES);
}

void Rectangle::freeLiveVertices() {
	vkQueueWaitIdle(mountain->queue);
	vmaDestroyBuffer(mountain->allocator, vertexBuffer, vertexAllocation);
	aspects.drop(RectangleAspect::LIVE_VERTICES);
}
