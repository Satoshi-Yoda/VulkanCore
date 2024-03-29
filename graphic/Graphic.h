#pragma once

#include <array>
#include <bitset>
#include <set>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vk_mem_alloc.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../core/Crater.h"
#include "../core/Lava.h"
#include "../core/Rocks.h"
#include "../state/flag_group.hpp"
#include "GraphicLayout.h"

using std::array;
using std::bitset;
using std::set;
using std::string;
using std::vector;

using glm::vec2;
using glm::vec3;

class Lava;

enum class GraphicAspect {
	WORKING_VERTICES,
	WORKING_DATA,
	STAGING_VERTICES,
	STAGING_DATA,
	LIVE_VERTICES,
	LIVE_DATA,
};

class Graphic : public std::enable_shared_from_this<Graphic> {
public:
	Graphic(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava);
	Graphic(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava, GraphicStyle style);
	~Graphic();

	Graphic(const Graphic&)            = delete;
	Graphic(Graphic&&)                 = delete;
	Graphic& operator=(const Graphic&) = delete;
	Graphic& operator=(Graphic&&)      = delete;

	GraphicStyle style;

	flag_group<GraphicAspect> aspects;

	vector<GraphicVertex> vertices;
	vec2 position { 0, 0 };

	uint32_t vertexCount = 0;
	VkBuffer vertexBuffer;
	VmaAllocation vertexAllocation;
	VmaAllocationInfo vertexInfo;
	uint32_t stagingVertexCount = 0;
	VkBuffer stagingVertexBuffer;
	VmaAllocation stagingVertexAllocation;
	VmaAllocationInfo stagingVertexInfo;

	vector<float> points;
	array<float, 2> transform = { 1.0f, 0.0f };
	size_t maxResolution = 16384;

	GraphicData data;
	VkBuffer dataBuffer;
	VmaAllocation dataAllocation;
	VmaAllocationInfo dataInfo;
	VkBuffer stagingDataBuffer;
	size_t stagingDataCount = 0;
	VmaAllocation stagingDataAllocation;
	VmaAllocationInfo stagingDataInfo;

	VkDescriptorSet descriptorSet;

	void establish(GraphicAspect aspect);
	template <typename... Args>
	void establish(GraphicAspect aspect, Args... args) {
		establish(aspect);
		establish(args...);
	}

	void establish(VkCommandBuffer cb, GraphicAspect aspect);
	template <typename... Args>
	void establish(VkCommandBuffer cb, GraphicAspect aspect, Args... args) {
		establish(cb, aspect);
		establish(cb, args...);
	}

	void refresh(GraphicAspect aspect);
	template <typename... Args>
	void refresh(GraphicAspect aspect, Args... args) {
		refresh(aspect);
		refresh(args...);
	}

	void createDescriptorSet();

	void free(GraphicAspect aspect);
	template <typename... Args>
	void free(GraphicAspect aspect, Args... args) {
		free(aspect);
		free(args...);
	}

	void paint();
	void refresh();

private:
	Ash& ash;
	Mountain& mountain;
	Rocks& rocks;
	Crater& crater;
	Lava& lava;

	void establishStagingVertices();
	void establishStagingData();
	void establishLiveVertices (VkCommandBuffer externalCommandBuffer = nullptr);
	void establishLiveData     (VkCommandBuffer externalCommandBuffer = nullptr);

	void refreshDataFromRaw();
	void refreshWorkingVertices();
	void refreshStagingVertices();
	void refreshStagingData();
	void refreshLiveVertices (VkCommandBuffer externalCommandBuffer = nullptr);
	void refreshLiveData     (VkCommandBuffer externalCommandBuffer = nullptr);

	void freeStagingVertices();
	void freeStagingData();
	void freeLiveVertices();
	void freeLiveData();
};
