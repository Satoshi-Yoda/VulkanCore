#pragma once

#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Mountain.h"
#include "Rocks.h"

using std::vector;

class Crater {
public:
	Crater(Ash &ash, Mountain &mountain, Rocks &rocks);
	~Crater();

	const bool DISPLAY_CHAIN_SIZE        = false;
	const bool DISPLAY_BIT_DEPTH         = false;
	const bool DISPLAY_RESOLUTION        = false;
	const bool DISPLAY_PRESENT_TRANSFORM = false;
	const bool DISPLAY_PRESENT_MODES     = false;

	const bool USE_GAMMA_CORRECT  = false;
	const bool USE_10_BIT         = false;
	const bool USE_VSYNC          = false;

	const size_t ADDITIONAL_CHAIN_SIZE = 1;

	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkSurfaceFormatKHR surfaceFormat;
	VkExtent2D extent;
	vector<VkImage> images;
	vector<VkImageView> imageViews;

	void reinit();

private:
	Ash& ash;
	Mountain& mountain;
	Rocks& rocks;

	VkSurfaceCapabilitiesKHR capabilities;
	vector<VkSurfaceFormatKHR> availableFormats;
	vector<VkPresentModeKHR> availablePresentModes;
	uint32_t chainSize;
	VkImageUsageFlags imageUsageFlags;
	VkSurfaceTransformFlagBitsKHR presentTransform;
	VkPresentModeKHR presentMode;

	void init();
	void clear();

	void querySurfaceCapabilities();
	void querySurfaceFormats();
	void querySurfacePresentModes();

	void chooseChainSize();
	void chooseSurfaceFormat();
	void chooseExtent();
	void chooseImageUsageFlags();
	void choosePresentTransform();
	void choosePresentMode();

	void createSwapChain();
	void queryImages();
	void createImageViews();
};
