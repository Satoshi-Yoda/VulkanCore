#include "Rectangle.h"

#include <cassert>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../utils/Loader.h"

using glm::vec2;
using glm::vec3;

using namespace std;

// TODO crater isn't used
Rectangle::Rectangle(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) : ash(ash), mountain(mountain), rocks(rocks), crater(crater), lava(lava) { }

Rectangle::~Rectangle() {
	if (this->aspects.has(RectangleAspect::STAGING_VERTICES)) freeStagingVertices();
	if (this->aspects.has(RectangleAspect::STAGING_DATA))     freeStagingData();
	if (this->aspects.has(RectangleAspect::LIVE_VERTICES))    freeLiveVertices();
	if (this->aspects.has(RectangleAspect::LIVE_DATA))        freeLiveData();
}

void Rectangle::setWorkingData(vector<RectangleVertex> vertices, RectangleData rectangleData) {
	this->vertices = vertices;
	this->data = rectangleData;
	aspects.raise(RectangleAspect::WORKING_VERTICES);
	aspects.raise(RectangleAspect::WORKING_DATA);
}

void Rectangle::establish(RectangleAspect aspect) {
	if (aspect == RectangleAspect::STAGING_VERTICES) establishStagingVertices();
	if (aspect == RectangleAspect::STAGING_DATA)     establishStagingData();
	if (aspect == RectangleAspect::LIVE_VERTICES)    establishLiveVertices();
	if (aspect == RectangleAspect::LIVE_DATA)        establishLiveData();
}

void Rectangle::establish(VkCommandBuffer cb, RectangleAspect aspect) {
	if (aspect == RectangleAspect::LIVE_VERTICES) establishLiveVertices(cb);
	if (aspect == RectangleAspect::LIVE_DATA)     establishLiveData(cb);
}

void Rectangle::refresh(RectangleAspect aspect) { }

void Rectangle::createDescriptorSet() {
	VkDescriptorSetAllocateInfo allocInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = mountain.descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &lava.rectangleLayout.descriptorSetLayout;

	vkAllocateDescriptorSets(mountain.device, &allocInfo, &descriptorSet) >> ash("Failed to allocate descriptor set!");

	VkDescriptorBufferInfo uniformInfo {};
	uniformInfo.buffer = lava.uniformBuffer;
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
	dataInfo.range = sizeof(RectangleData);

	VkWriteDescriptorSet dataWrite { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	dataWrite.dstSet = descriptorSet;
	dataWrite.dstBinding = 1;
	dataWrite.dstArrayElement = 0;
	dataWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	dataWrite.descriptorCount = 1;
	dataWrite.pBufferInfo = &dataInfo;

	array<VkWriteDescriptorSet, 2> descriptorWrites { uniformWrite, dataWrite };

	// can be used to update several descriptor sets at once
	vkUpdateDescriptorSets(mountain.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Rectangle::free(RectangleAspect aspect) {
	if (aspect == RectangleAspect::STAGING_VERTICES && this->aspects.has(aspect)) freeStagingVertices();
	if (aspect == RectangleAspect::STAGING_DATA     && this->aspects.has(aspect)) freeStagingData();
	if (aspect == RectangleAspect::LIVE_VERTICES    && this->aspects.has(aspect)) freeLiveVertices();
	if (aspect == RectangleAspect::LIVE_DATA        && this->aspects.has(aspect)) freeLiveData();
}

void Rectangle::establishStagingVertices() {
	#ifdef use_validation
	aspects.has(RectangleAspect::WORKING_VERTICES) >> ash("In this rectangle there is no WORKING_VERTICES");
	#endif

	stagingVertexCount = static_cast<uint32_t>(vertices.size());
	VkDeviceSize bufferSize = sizeof(RectangleVertex) * vertices.size();

	rocks.createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingVertexBuffer, stagingVertexAllocation, stagingVertexInfo);
	memcpy(stagingVertexInfo.pMappedData, vertices.data(), static_cast<size_t>(bufferSize));

	aspects.raise(RectangleAspect::STAGING_VERTICES);
}

void Rectangle::establishStagingData() {
	#ifdef use_validation
	aspects.has(RectangleAspect::WORKING_DATA) >> ash("In this rectangle there is no WORKING_DATA");
	#endif

	VkDeviceSize bufferSize = sizeof(RectangleData);

	rocks.createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingDataBuffer, stagingDataAllocation, stagingDataInfo);
	memcpy(stagingDataInfo.pMappedData, &data, static_cast<size_t>(bufferSize));

	aspects.raise(RectangleAspect::STAGING_DATA);
}

void Rectangle::establishLiveVertices(VkCommandBuffer externalCommandBuffer) {
	#ifdef use_validation
	aspects.has(RectangleAspect::STAGING_VERTICES) >> ash("In this rectangle there is no STAGING_VERTICES");
	#endif

	VkDeviceSize bufferSize = sizeof(RectangleVertex) * stagingVertexCount;
	vertexCount = stagingVertexCount;

	rocks.createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, vertexBuffer, vertexAllocation, vertexInfo);

	bool useExternalCommandBuffer = (externalCommandBuffer != nullptr);
	VkCommandBuffer commandBuffer = useExternalCommandBuffer ? externalCommandBuffer : rocks.beginSingleTimeCommands();

	rocks.copyBufferToBuffer(stagingVertexBuffer, vertexBuffer, bufferSize, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, commandBuffer);

	if (useExternalCommandBuffer == false) {
		rocks.endSingleTimeCommands(commandBuffer);
	}

	aspects.raise(RectangleAspect::LIVE_VERTICES);
}

void Rectangle::establishLiveData(VkCommandBuffer externalCommandBuffer) {
	#ifdef use_validation
	aspects.has(RectangleAspect::STAGING_DATA) >> ash("In this rectangle there is no STAGING_DATA");
	#endif

	VkDeviceSize bufferSize = sizeof(RectangleData);

	rocks.createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, dataBuffer, dataAllocation, dataInfo);

	bool useExternalCommandBuffer = (externalCommandBuffer != nullptr);
	VkCommandBuffer commandBuffer = useExternalCommandBuffer ? externalCommandBuffer : rocks.beginSingleTimeCommands();

	rocks.copyBufferToBuffer(stagingDataBuffer, dataBuffer, bufferSize, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, commandBuffer);

	if (useExternalCommandBuffer == false) {
		rocks.endSingleTimeCommands(commandBuffer);
	}

	aspects.raise(RectangleAspect::LIVE_DATA);
}

void Rectangle::freeStagingVertices() {
	// TODO can be optimized by calling this later in future frames, without wait, or something
	vkQueueWaitIdle(mountain.queue); // TODO do I need this for staging resources?
	vmaDestroyBuffer(mountain.allocator, stagingVertexBuffer, stagingVertexAllocation);
	aspects.drop(RectangleAspect::STAGING_VERTICES);
}

void Rectangle::freeStagingData() {
	vkQueueWaitIdle(mountain.queue);
	vmaDestroyBuffer(mountain.allocator, stagingDataBuffer, stagingDataAllocation);
	aspects.drop(RectangleAspect::STAGING_DATA);
}

void Rectangle::freeLiveVertices() {
	vkQueueWaitIdle(mountain.queue);
	vmaDestroyBuffer(mountain.allocator, vertexBuffer, vertexAllocation);
	aspects.drop(RectangleAspect::LIVE_VERTICES);
}

void Rectangle::freeLiveData() {
	vkQueueWaitIdle(mountain.queue);
	vmaDestroyBuffer(mountain.allocator, dataBuffer, dataAllocation);
	aspects.drop(RectangleAspect::LIVE_DATA);
}

void Rectangle::paint() {
	int x = round(position.x);
	int y = round(position.y);
	int w = round(data.size.x);
	int h = round(data.size.y);
	vector<RectangleVertex> vertices;

	int x_min = x - w / 2;
	int x_max = x_min + w;
	int y_min = y - h / 2;
	int y_max = y_min + h;

	vertices.push_back({ { x_min, y_max }, { 0 - 0.5, h + 0.5 } });
	vertices.push_back({ { x_max, y_max }, { w + 0.5, h + 0.5 } });
	vertices.push_back({ { x_min, y_min }, { 0 - 0.5, 0 - 0.5 } });

	vertices.push_back({ { x_max, y_max }, { w + 0.5, h + 0.5 } });
	vertices.push_back({ { x_max, y_min }, { w + 0.5, 0 - 0.5 } });
	vertices.push_back({ { x_min, y_min }, { 0 - 0.5, 0 - 0.5 } });

	setWorkingData(vertices, data);

	establish(RectangleAspect::STAGING_VERTICES, RectangleAspect::STAGING_DATA);
	establish(RectangleAspect::LIVE_VERTICES, RectangleAspect::LIVE_DATA);
	free(RectangleAspect::STAGING_VERTICES, RectangleAspect::STAGING_DATA);
	createDescriptorSet();

	lava.addRectangle(shared_from_this());
}

void Rectangle::refresh() {
	free(RectangleAspect::LIVE_VERTICES, RectangleAspect::LIVE_DATA);

	int x = round(position.x);
	int y = round(position.y);
	int w = round(data.size.x);
	int h = round(data.size.y);
	vector<RectangleVertex> vertices;

	int x_min = x - w / 2;
	int x_max = x_min + w;
	int y_min = y - h / 2;
	int y_max = y_min + h;

	vertices.push_back({ { x_min, y_max }, { 0 - 0.5, h + 0.5 } });
	vertices.push_back({ { x_max, y_max }, { w + 0.5, h + 0.5 } });
	vertices.push_back({ { x_min, y_min }, { 0 - 0.5, 0 - 0.5 } });

	vertices.push_back({ { x_max, y_max }, { w + 0.5, h + 0.5 } });
	vertices.push_back({ { x_max, y_min }, { w + 0.5, 0 - 0.5 } });
	vertices.push_back({ { x_min, y_min }, { 0 - 0.5, 0 - 0.5 } });

	setWorkingData(vertices, data);

	establish(RectangleAspect::STAGING_VERTICES, RectangleAspect::STAGING_DATA);
	establish(RectangleAspect::LIVE_VERTICES, RectangleAspect::LIVE_DATA);
	free(RectangleAspect::STAGING_VERTICES, RectangleAspect::STAGING_DATA);
	createDescriptorSet();
}
