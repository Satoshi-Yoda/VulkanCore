#include "Graphic.h"

#include <cassert>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../utils/Loader.h"

using glm::vec2;
using glm::vec3;

using namespace std;

Graphic::Graphic(Ash& ash) : ash(ash) { }

Graphic::~Graphic() {
	if (this->aspects.has(GraphicAspect::STAGING_VERTICES)) freeStagingVertices();
	if (this->aspects.has(GraphicAspect::STAGING_DATA))     freeStagingData();
	if (this->aspects.has(GraphicAspect::LIVE_VERTICES))    freeLiveVertices();
	if (this->aspects.has(GraphicAspect::LIVE_DATA))        freeLiveData();
}

void Graphic::setName(string name) {
	this->name = name;
}

void Graphic::setWorkingData(vector<GraphicVertex> vertices, GraphicData graphicData) {
	this->vertices = vertices;
	this->data = graphicData;
	aspects.raise(GraphicAspect::WORKING_VERTICES);
	aspects.raise(GraphicAspect::WORKING_DATA);
}

void Graphic::setVulkanEntities(Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) {
	this->mountain = &mountain;
	this->rocks = &rocks;
	this->crater = &crater;
	this->lava = &lava;
	aspects.raise(GraphicAspect::VULKAN_ENTITIES);
}

void Graphic::establish(GraphicAspect aspect) {
	if (aspect == GraphicAspect::STAGING_VERTICES) establishStagingVertices();
	if (aspect == GraphicAspect::STAGING_DATA)     establishStagingData();
	if (aspect == GraphicAspect::LIVE_VERTICES)    establishLiveVertices();
	if (aspect == GraphicAspect::LIVE_DATA)        establishLiveData();
}

void Graphic::establish(VkCommandBuffer cb, GraphicAspect aspect) {
	if (aspect == GraphicAspect::LIVE_VERTICES) establishLiveVertices(cb);
	if (aspect == GraphicAspect::LIVE_DATA)     establishLiveData(cb);
}

void Graphic::refresh(GraphicAspect aspect) { }

void Graphic::createDescriptorSet() {
	assert(mountain != nullptr);

	VkDescriptorSetAllocateInfo allocInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = mountain->descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &lava->graphicLayout.descriptorSetLayout;

	vkAllocateDescriptorSets(mountain->device, &allocInfo, &descriptorSet) >> ash("Failed to allocate descriptor set!");

	VkDescriptorBufferInfo uniformInfo {};
	uniformInfo.buffer = lava->uniformBuffer;
	uniformInfo.offset = 0;
	uniformInfo.range = sizeof(UniformBufferObject);

	VkWriteDescriptorSet uniformWrite { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	uniformWrite.dstSet = descriptorSet;
	uniformWrite.dstBinding = 0;
	uniformWrite.dstArrayElement = 0;
	uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformWrite.descriptorCount = 1;
	uniformWrite.pBufferInfo = &uniformInfo;

	VkDescriptorBufferInfo dataInfo {};
	dataInfo.buffer = dataBuffer;
	dataInfo.offset = 0;
	dataInfo.range = sizeof(GraphicData);

	VkWriteDescriptorSet dataWrite { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	dataWrite.dstSet = descriptorSet;
	dataWrite.dstBinding = 1;
	dataWrite.dstArrayElement = 0;
	dataWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	dataWrite.descriptorCount = 1;
	dataWrite.pBufferInfo = &dataInfo;

	array<VkWriteDescriptorSet, 2> descriptorWrites { uniformWrite, dataWrite };

	// can be used to update several descriptor sets at once
	vkUpdateDescriptorSets(mountain->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Graphic::free(GraphicAspect aspect) {
	if (aspect == GraphicAspect::STAGING_VERTICES && this->aspects.has(aspect)) freeStagingVertices();
	if (aspect == GraphicAspect::STAGING_DATA     && this->aspects.has(aspect)) freeStagingData();
	if (aspect == GraphicAspect::LIVE_VERTICES    && this->aspects.has(aspect)) freeLiveVertices();
	if (aspect == GraphicAspect::LIVE_DATA        && this->aspects.has(aspect)) freeLiveData();
}

void Graphic::establishStagingVertices() {
	#ifdef use_validation
	aspects.has(GraphicAspect::WORKING_VERTICES, GraphicAspect::VULKAN_ENTITIES) >> ash("In this graphic there is no WORKING_VERTICES or no VULKAN_ENTITIES");
	#endif

	stagingVertexCount = static_cast<uint32_t>(vertices.size());
	VkDeviceSize bufferSize = sizeof(GraphicVertex) * vertices.size();

	rocks->createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingVertexBuffer, stagingVertexAllocation, stagingVertexInfo);
	memcpy(stagingVertexInfo.pMappedData, vertices.data(), static_cast<size_t>(bufferSize));

	aspects.raise(GraphicAspect::STAGING_VERTICES);
}

void Graphic::establishStagingData() {
	#ifdef use_validation
	aspects.has(GraphicAspect::WORKING_DATA, GraphicAspect::VULKAN_ENTITIES) >> ash("In this graphic there is no WORKING_DATA or no VULKAN_ENTITIES");
	#endif

	VkDeviceSize bufferSize = sizeof(GraphicData);

	rocks->createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingDataBuffer, stagingDataAllocation, stagingDataInfo);
	memcpy(stagingDataInfo.pMappedData, &data, static_cast<size_t>(bufferSize));

	aspects.raise(GraphicAspect::STAGING_DATA);
}

void Graphic::establishLiveVertices(VkCommandBuffer externalCommandBuffer) {
	#ifdef use_validation
	aspects.has(GraphicAspect::STAGING_VERTICES, GraphicAspect::VULKAN_ENTITIES) >> ash("In this graphic there is no STAGING_VERTICES or no VULKAN_ENTITIES");
	#endif

	VkDeviceSize bufferSize = sizeof(GraphicVertex) * stagingVertexCount;
	vertexCount = stagingVertexCount;

	rocks->createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, vertexBuffer, vertexAllocation, vertexInfo);

	bool useExternalCommandBuffer = (externalCommandBuffer != nullptr);
	VkCommandBuffer commandBuffer = useExternalCommandBuffer ? externalCommandBuffer : rocks->beginSingleTimeCommands();

	rocks->copyBufferToBuffer(stagingVertexBuffer, vertexBuffer, bufferSize, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, commandBuffer);

	if (useExternalCommandBuffer == false) {
		rocks->endSingleTimeCommands(commandBuffer);
	}

	aspects.raise(GraphicAspect::LIVE_VERTICES);
}

void Graphic::establishLiveData(VkCommandBuffer externalCommandBuffer) {
	#ifdef use_validation
	aspects.has(GraphicAspect::STAGING_DATA, GraphicAspect::VULKAN_ENTITIES) >> ash("In this graphic there is no STAGING_DATA or no VULKAN_ENTITIES");
	#endif

	VkDeviceSize bufferSize = sizeof(GraphicData);

	rocks->createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, dataBuffer, dataAllocation, dataInfo);

	bool useExternalCommandBuffer = (externalCommandBuffer != nullptr);
	VkCommandBuffer commandBuffer = useExternalCommandBuffer ? externalCommandBuffer : rocks->beginSingleTimeCommands();

	rocks->copyBufferToBuffer(stagingDataBuffer, dataBuffer, bufferSize, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, commandBuffer);

	if (useExternalCommandBuffer == false) {
		rocks->endSingleTimeCommands(commandBuffer);
	}

	aspects.raise(GraphicAspect::LIVE_DATA);
}

void Graphic::freeStagingVertices() {
	// TODO can be optimized by calling this later in future frames, without wait, or something
	vkQueueWaitIdle(mountain->queue); // TODO do I need this for staging resources?
	vmaDestroyBuffer(mountain->allocator, stagingVertexBuffer, stagingVertexAllocation);
	aspects.drop(GraphicAspect::STAGING_VERTICES);
}

void Graphic::freeStagingData() {
	vkQueueWaitIdle(mountain->queue);
	vmaDestroyBuffer(mountain->allocator, stagingDataBuffer, stagingDataAllocation);
	aspects.drop(GraphicAspect::STAGING_DATA);
}

void Graphic::freeLiveVertices() {
	vkQueueWaitIdle(mountain->queue);
	vmaDestroyBuffer(mountain->allocator, vertexBuffer, vertexAllocation);
	aspects.drop(GraphicAspect::LIVE_VERTICES);
}

void Graphic::freeLiveData() {
	vkQueueWaitIdle(mountain->queue);
	vmaDestroyBuffer(mountain->allocator, dataBuffer, dataAllocation);
	aspects.drop(GraphicAspect::LIVE_DATA);
}
