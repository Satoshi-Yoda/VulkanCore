#ifndef TECTONIC_H
#define TECTONIC_H

#include <vector>
#include <array>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Mountain.h"
#include "Crater.h"
#include "Lava.h"

using std::vector;
using std::array;

class Tectonic {
public:
	Tectonic(Ash &ash, Mountain &mountain, Rocks &rocks, Crater &crater, Lava &lava);
	~Tectonic();

	void drawFrame();

private:
	Ash& ash;
	Mountain& mountain;
	Rocks &rocks;
	Crater& crater;
	Lava &lava;

	const static int IN_FLIGHT_FRAMES = 2;
	int inFlightIndex = 0;

	array<VkCommandBuffer, IN_FLIGHT_FRAMES> commandBuffers;
	array<VkSemaphore,     IN_FLIGHT_FRAMES> imageAvailableSemaphores;
	array<VkSemaphore,     IN_FLIGHT_FRAMES> renderFinishedSemaphores;
	array<VkFence,         IN_FLIGHT_FRAMES> fences;
	array<VkFramebuffer,   IN_FLIGHT_FRAMES> framebuffers;
	array<VkBuffer,        IN_FLIGHT_FRAMES> uniformBuffers;
	array<VkDeviceMemory,  IN_FLIGHT_FRAMES> uniformBuffersMemory;
	vector<array<VkDescriptorSet, IN_FLIGHT_FRAMES>> descriptorSets;

	void createInFlightResources();
	void createUniformBuffers();
	void resizeDescriptorSets(size_t size);
	void updateInFlightUniformBuffer();
	void updateInFlightDescriptorSet(size_t imageIndex, VkImageView& imageView);
	void prepareFrame(uint32_t craterIndex);
};

#endif