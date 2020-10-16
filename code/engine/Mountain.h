#ifndef MOUNTAIN_H
#define MOUNTAIN_H

#include <iostream>
#include <tuple>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Ash.h"

using std::vector;
using std::tuple;

class Mountain {
public:
	GLFWwindow* window;
	VkSurfaceKHR surface;
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue queue;
	VkCommandPool commandPool;
	VkDescriptorPool descriptorPool;

	// public for now, not used here!
	bool framebufferResized = false;

	const bool DISPLAY_LAYERS              = false;
	const bool DISPLAY_INSTANCE_EXTENSIONS = false;
	const bool DISPLAY_DEVICE_EXTENSIONS   = false;
	const bool DISPLAY_DEVICES             = false;
	const bool DISPLAY_QUEUES              = false;

	const bool USE_FULLSCREEN        = false;
	const bool USE_BORDERLESS_WINDOW = false;
	const bool USE_SAMPLE_SHADING    = false;
	const bool USE_VALIDATION_LAYERS = false;

	Mountain(Ash &ash);
	Mountain(const Mountain&) = delete;
	Mountain& operator=(const Mountain&) = delete;
	~Mountain();

	void showWindow();

private:
	Ash& ash;

	uint32_t windowWidth  = 2048;
	uint32_t windowHeight = 1536;
	uint32_t queueFamilyIndex;
	VkDebugUtilsMessengerEXT debugMessenger;

	const vector<const char*> VALIDATION_LAYERS { "VK_LAYER_KHRONOS_validation" };
	const vector<const char*> DEVICE_EXTENSIONS { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	void initWindow();

	void createInstance();
	void createSurface();
	void createDevice();
	void createCommandPool();
	void createDescriptorPool();

	bool isDeviceSuitable(VkPhysicalDevice physicalDevice);
	string queryDeviceMemoryInfo(VkPhysicalDevice physicalDevice);
	tuple<bool, uint32_t, string> findQueueFamilies(VkPhysicalDevice physicalDevice);

	void setupDebugMessenger();
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	vector<const char*> getRequiredExtensions();
	bool checkRequiredExtensionsSupport(vector<const char*> &requiredExtensions);
	void checkValidationLayerSupport();
	bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
};

#endif