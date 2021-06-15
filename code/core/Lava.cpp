#include "Lava.h"
#include "../utils/Loader.h"

#include <array>
#include <vector>

using namespace std;

Lava::Lava(Ash &ash, Mountain &mountain, Rocks &rocks, Crater &crater) : ash(ash), mountain(mountain), rocks(rocks), crater(crater) {
	createUniformBuffers();
}

Lava::~Lava() {
	if (mountain.device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(mountain.device);

		vmaDestroyBuffer(mountain.allocator, uniformBuffer, uniformBuffersAllocation);
	}
}

void Lava::createUniformBuffers() {
	rocks.createBufferVMA(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, uniformBuffer, uniformBuffersAllocation, uniformBuffersAllocationInfo);
}

void Lava::addCave(unique_ptr<Cave> cave) {
	this->caves.push_back(move(cave));
}
