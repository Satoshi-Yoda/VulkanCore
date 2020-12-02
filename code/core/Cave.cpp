#include "cave.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

using glm::vec2;
using glm::vec3;

using namespace std;

Cave::Cave(Rocks& rocks, vector<Vertex> vertices, int width, int height, void* pixels) : vertices(vertices), width(width), height(height), pixels(pixels), rocks(rocks) { }

Cave::~Cave() {
	// TODO free all allocated things
}

bool Cave::has(CaveAspects aspects) {
	return (this->aspects & aspects) == aspects;
}

void Cave::establish(CaveAspects aspects) {
	// TODO implement
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
	if (!has(CaveAspects::WORKING_VERTICES)) {
		throw logic_error("In this cave there is no WORKING_VERTICES");
	}
	#endif

	vertexCount = static_cast<uint32_t>(vertices.size());
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

	rocks.createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingVertexBuffer, stagingVertexAllocation, stagingVertexInfo);
	memcpy(stagingVertexInfo.pMappedData, vertices.data(), static_cast<size_t>(bufferSize));

	aspects |= CaveAspects::STAGING_VERTICES;
}

void Cave::establishStagingInstances() {
	#ifdef use_validation
	if (!has(CaveAspects::WORKING_INSTANCES)) {
		throw logic_error("In this cave there is no WORKING_INSTANCES");
	}
	#endif

	instanceCount = static_cast<uint32_t>(instances.size());
	VkDeviceSize bufferSize = sizeof(Instance) * instances.size();

	if (instanceCount == 0) return;

	rocks.createBufferVMA(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingInstanceBuffer, stagingInstanceAllocation, stagingInstanceInfo);
	memcpy(stagingInstanceInfo.pMappedData, instances.data(), static_cast<size_t>(bufferSize));

	aspects |= CaveAspects::STAGING_INSTANCES;
}

void Cave::establishStagingTexture() {
		#ifdef use_validation
	if (!has(CaveAspects::WORKING_TEXTURE)) {
		throw logic_error("In this cave there is no WORKING_TEXTURE");
	}
	#endif

	VkDeviceSize imageSize = width * height * 4;

	rocks.createBufferVMA(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingTextureBuffer, stagingTextureAllocation, stagingTextureInfo);
	memcpy(stagingTextureInfo.pMappedData, pixels, static_cast<size_t>(imageSize));

	aspects |= CaveAspects::STAGING_TEXTURE;
}

void Cave::establishLiveVertices() {

}

void Cave::establishLiveInstances() {

}

void Cave::establishLiveTexture() {

}
