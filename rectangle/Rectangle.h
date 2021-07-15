#pragma once

#include <bitset>
#include <set>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../core/Crater.h"
#include "../core/Lava.h"
#include "../core/Rocks.h"
#include "../state/flag_group.hpp"
#include "RectangleLayout.h"

using std::bitset;
using std::set;
using std::string;
using std::vector;

using glm::vec2;
using glm::vec3;

class Lava;

enum class RectangleAspect {
	WORKING_VERTICES,
	WORKING_DATA,
	STAGING_VERTICES,
	STAGING_DATA,
	LIVE_VERTICES,
	LIVE_DATA,
};

class Rectangle : public std::enable_shared_from_this<Rectangle> {
public:
	Rectangle(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava);
	~Rectangle();

	Rectangle(const Rectangle&)            = delete;
	Rectangle(Rectangle&&)                 = delete;
	Rectangle& operator=(const Rectangle&) = delete;
	Rectangle& operator=(Rectangle&&)      = delete;

	void setName(string name);
	void setWorkingData(vector<RectangleVertex> vertices, RectangleData rectangleData);   // TODO maybe move this to constructor?

	flag_group<RectangleAspect> aspects;

	string name;
	vector<RectangleVertex> vertices;
	vec2 position { 0, 0 };

	uint32_t vertexCount = 0;
	VkBuffer vertexBuffer;
	VmaAllocation vertexAllocation;
	VmaAllocationInfo vertexInfo;
	uint32_t stagingVertexCount = 0;
	VkBuffer stagingVertexBuffer;
	VmaAllocation stagingVertexAllocation;
	VmaAllocationInfo stagingVertexInfo;

	RectangleData data;
	VkBuffer dataBuffer;
	VmaAllocation dataAllocation;
	VmaAllocationInfo dataInfo;
	VkBuffer stagingDataBuffer;
	VmaAllocation stagingDataAllocation;
	VmaAllocationInfo stagingDataInfo;

	VkDescriptorSet descriptorSet;

	void establish(RectangleAspect aspect);
	template <typename... Args>
	void establish(RectangleAspect aspect, Args... args) {
		establish(aspect);
		establish(args...);
	}

	void establish(VkCommandBuffer cb, RectangleAspect aspect);
	template <typename... Args>
	void establish(VkCommandBuffer cb, RectangleAspect aspect, Args... args) {
		establish(cb, aspect);
		establish(cb, args...);
	}

	void refresh(RectangleAspect aspect);
	template <typename... Args>
	void refresh(RectangleAspect aspect, Args... args) {
		refresh(aspect);
		refresh(args...);
	}

	void createDescriptorSet();

	void free(RectangleAspect aspect);
	template <typename... Args>
	void free(RectangleAspect aspect, Args... args) {
		free(aspect);
		free(args...);
	}

	void paint();

private:
	Ash& ash;
	Mountain& mountain;
	Rocks& rocks;
	Crater& crater;
	Lava& lava;

	void establishStagingVertices();
	void establishStagingData();
	void refreshStagingInstances();
	void establishLiveVertices (VkCommandBuffer externalCommandBuffer = nullptr);
	void establishLiveData     (VkCommandBuffer externalCommandBuffer = nullptr);
	void refreshLiveInstances  (VkCommandBuffer externalCommandBuffer = nullptr);

	void freeStagingVertices();
	void freeStagingData();
	void freeLiveVertices();
	void freeLiveData();
};
