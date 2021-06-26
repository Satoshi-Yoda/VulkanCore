#include "Mountain.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <array>
#include <iostream>
#include <set>
#include <tuple>
#include <vector>

using namespace std;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		string severity = "[NONE]";
		switch (messageSeverity) {
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:    severity = "[INFO]";    break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: severity = "[VERBOSE]"; break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: severity = "[WARNING]"; break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:   severity = "[ERROR]";   break;
			default:                                              severity = "[DEFAULT]"; break;
		}
		cerr << severity << " Validation layer: " << pCallbackData->pMessage << endl << endl;
	}
	return VK_FALSE;
}

void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<Mountain*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

Mountain::Mountain(Ash &ash) : ash(ash) {
	initWindow();
	createInstance();
	setupDebugMessenger();
	createSurface();
	createDevice();
	createAllocator();
	createCommandPool();
	createDescriptorPool();
}

Mountain::~Mountain() {
	if (device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(device);
	}

	if (commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(device, commandPool, nullptr);
	if (descriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, descriptorPool, nullptr);

	vmaDestroyAllocator(allocator);

	if (device != VK_NULL_HANDLE) vkDestroyDevice(device, nullptr);
	if (USE_VALIDATION_LAYERS) DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	if (instance != VK_NULL_HANDLE && surface != VK_NULL_HANDLE) vkDestroySurfaceKHR(instance, surface, nullptr);
	if (instance != VK_NULL_HANDLE) vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Mountain::initWindow() {
	glfwInit();
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	if (USE_BORDERLESS_WINDOW) glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	if (USE_FULLSCREEN) {
		window = glfwCreateWindow(mode->width, mode->height, "Mountain", glfwGetPrimaryMonitor(), NULL);
	} else {
		window = glfwCreateWindow(windowWidth, windowHeight, "Mountain", nullptr, nullptr);
	}
	(window != NULL) >> ash("Failed to create window!");
	glfwSetWindowPos(window, (mode->width - windowWidth) / 3, (mode->height - windowHeight) / 2);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Mountain::showWindow() {
	glfwShowWindow(window);
}

void Mountain::createSurface() {
	glfwCreateWindowSurface(instance, window, nullptr, &surface) >> ash("Failed to create window surface!");
}

VkResult Mountain::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Mountain::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void Mountain::setupDebugMessenger() {
	if (USE_VALIDATION_LAYERS == false) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) >> ash("Failed to setup debug messenger!");
}

void Mountain::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

void Mountain::createInstance() {
	if (USE_VALIDATION_LAYERS) checkValidationLayerSupport();

	VkApplicationInfo appInfo { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName = "VulkanCore";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.pEngineName = "Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 71);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	auto requiredExtensions = getRequiredExtensions();
	checkRequiredExtensionsSupport(requiredExtensions) >> ash("Required extensions not supported!");

	VkInstanceCreateInfo instanceCreateInfo { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (USE_VALIDATION_LAYERS) {
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		instanceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		populateDebugMessengerCreateInfo(debugCreateInfo);
		instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
	} else {
		instanceCreateInfo.enabledLayerCount = 0;
	}

	vkCreateInstance(&instanceCreateInfo, nullptr, &instance) >> ash("Failed to create Vulkan instance!");
}

bool Mountain::checkRequiredExtensionsSupport(vector<const char*> &requiredExtensions) {
	set<string> required { requiredExtensions.cbegin(), requiredExtensions.cend() };

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
	set<string> supported;

	if (DISPLAY_INSTANCE_EXTENSIONS) printf("Instance extensions:\n");
	for (auto extension : extensions) {
		supported.insert(extension.extensionName);
		if (required.find(extension.extensionName) != required.end()) {
			if (DISPLAY_INSTANCE_EXTENSIONS) printf(" + %s v%d\n", extension.extensionName, extension.specVersion);
		} else {
			if (DISPLAY_INSTANCE_EXTENSIONS) printf("   %s v%d\n", extension.extensionName, extension.specVersion);
		}
	}
	if (DISPLAY_INSTANCE_EXTENSIONS) printf("\n");

	bool result = true;
	for (auto layer : required) {
		if (supported.find(layer) == supported.end()) {
			result = false;
		}
	}
	return result;
}

vector<const char*> Mountain::getRequiredExtensions() {
	uint32_t requiredExtensionCount = 0;
	const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
	vector<const char*> result(requiredExtensions, requiredExtensions + requiredExtensionCount);

	// saves about 0.01 seconds xD because of some dynamic libraries loading
	// vector<const char*> result { "VK_KHR_surface", "VK_KHR_win32_surface" };

	if (USE_VALIDATION_LAYERS) {
	    result.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	return result;
}

void Mountain::checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	set<string> required { VALIDATION_LAYERS.cbegin(), VALIDATION_LAYERS.cend() };

	if (DISPLAY_LAYERS) printf("\nValidation layers:\n");
	set<string> available;
	for (auto layer : availableLayers) {
		available.insert(layer.layerName);
		if (required.find(layer.layerName) != required.end()) {
			if (DISPLAY_LAYERS) printf(" + %s sv%d iv%d (%s)\n", layer.layerName, layer.specVersion, layer.implementationVersion, layer.description);
		} else {
			if (DISPLAY_LAYERS) printf("   %s sv%d iv%d (%s)\n", layer.layerName, layer.specVersion, layer.implementationVersion, layer.description);
		}
	}
	if (DISPLAY_LAYERS) printf("\n");

	for (auto layer : required) {
		// TODO implement ash formatting "Validation layer %s not available!\n"
		(available.find(layer) != available.end()) >> ash("Validation layer not available!");
	}
}

void Mountain::createDevice() {
	uint32_t numDevices = 0;
	vkEnumeratePhysicalDevices(instance, &numDevices, nullptr) >> ash("Failed to get count of physical devices!");
	(numDevices != 0) >> ash("Zero physical devices found!");

	vector<VkPhysicalDevice> physicalDevices(numDevices);
	vkEnumeratePhysicalDevices(instance, &numDevices, physicalDevices.data()) >> ash("Failed to get physical devices!");

	for (uint32_t i = 0; i < numDevices; ++i) {
		if (isDeviceSuitable(physicalDevices[i])) {
			physicalDevice = physicalDevices[i];
			break;
		}
	}
	(physicalDevice != VK_NULL_HANDLE) >> ash("Failed to select physical device with required properties!");

	float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo queueCreateInfo { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = USE_SAMPLE_SHADING ? VK_TRUE : VK_FALSE;
	deviceFeatures.independentBlend = VK_TRUE; // TODO is not used here, allows independent blending for each color attachment

	VkDeviceCreateInfo deviceCreateInfo { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
	deviceCreateInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

	if (USE_VALIDATION_LAYERS) {
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		deviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
	} else {
		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = nullptr;
	}

	vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) >> ash("Failed to create logical device!");

	vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
	if (DISPLAY_QUEUES) printf("Selected and stored queue family index %d\n", queueFamilyIndex);
}

void Mountain::createAllocator() {
	VmaAllocatorCreateInfo allocatorInfo {};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.device = device;
	allocatorInfo.instance = instance;

	vmaCreateAllocator(&allocatorInfo, &allocator);
}

bool Mountain::checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
	vector<VkExtensionProperties> availableExtensions { extensionCount };
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	set<string> required { DEVICE_EXTENSIONS.cbegin(), DEVICE_EXTENSIONS.cend() };

	if (DISPLAY_DEVICE_EXTENSIONS) printf("Device extensions:\n");
	for (auto extension : availableExtensions) {
		if (required.find(extension.extensionName) != required.end()) {
			required.erase(extension.extensionName);
			if (DISPLAY_DEVICE_EXTENSIONS) printf(" + %s v%d\n", extension.extensionName, extension.specVersion);
		} else {
			if (DISPLAY_DEVICE_EXTENSIONS) printf("   %s v%d\n", extension.extensionName, extension.specVersion);
		}
	}
	if (DISPLAY_DEVICE_EXTENSIONS) printf("\n");

	return required.empty();
}

bool Mountain::isDeviceSuitable(VkPhysicalDevice physicalDevice) {
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	uint32_t apiMajor = VK_VERSION_MAJOR(deviceProperties.apiVersion);
	uint32_t apiMinor = VK_VERSION_MINOR(deviceProperties.apiVersion);
	uint32_t apiPatch = VK_VERSION_PATCH(deviceProperties.apiVersion);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	auto [successFamily, selectedFamily, familiesInfo] = findQueueFamilies(physicalDevice);

	bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

	bool result =
		apiMajor >= 1 &&
		successFamily &&
		extensionsSupported &&
		deviceFeatures.samplerAnisotropy;

	if (DISPLAY_DEVICES) {
		char apiVersion[100];
		sprintf(apiVersion, "v%d.%d.%d", apiMajor, apiMinor, apiPatch);
		string memoryInfo = queryDeviceMemoryInfo(physicalDevice);
		printf(" %s %s [%s] [%s] [%s]\n", (result ? "+" : " "), deviceProperties.deviceName, apiVersion, familiesInfo.data(), memoryInfo.data());
	}

	if (result) {
		queueFamilyIndex = selectedFamily;
	}

	return result;
}

string Mountain::queryDeviceMemoryInfo(VkPhysicalDevice physicalDevice) {
	VkPhysicalDeviceMemoryProperties deviceMemory;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemory);

	vector<VkMemoryHeap> memoryHeaps { deviceMemory.memoryHeaps, deviceMemory.memoryHeaps + deviceMemory.memoryHeapCount };
	vector<VkMemoryType> memoryTypes { deviceMemory.memoryTypes, deviceMemory.memoryTypes + deviceMemory.memoryTypeCount };

	uint32_t heapIndex = 0;
	string info = "";

	for (auto heap : memoryHeaps) {
		info.append((info != "") ? " " : "");
		char str[10];
		sprintf(str, "%.2fGB", static_cast<double>(heap.size) / (1 << 30));
		info.append(str);

		// TODO not strictly correct, but ok for now
		set<string> heapFeatures {};
		for (auto type : memoryTypes) {
			if (type.heapIndex == heapIndex) {
				auto f = type.propertyFlags;
				if (f & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)        heapFeatures.insert("-local");
				if (f & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)        heapFeatures.insert("-hostvisible");
				if (f & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)       heapFeatures.insert("-hostcoherent");
				if (f & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)         heapFeatures.insert("-hostcached");
				if (f & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)    heapFeatures.insert("-lazily");
				if (f & VK_MEMORY_PROPERTY_PROTECTED_BIT)           heapFeatures.insert("-protected");
				if (f & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) heapFeatures.insert("-amdcoherent");
				if (f & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) heapFeatures.insert("-amduncached");
			}
		}

		for (auto feature : heapFeatures) {
			info.append(feature);
		}

		heapIndex++;
	}

	int i = 0;
	for (auto type : memoryTypes) {
		info.append(" ").append(to_string(i++));
		auto f = type.propertyFlags;
		if (f & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)        info.append("-local");
		if (f & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)        info.append("-hostvisible");
		if (f & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)       info.append("-hostcoherent");
		if (f & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)         info.append("-hostcached");
		if (f & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)    info.append("-lazily");
		if (f & VK_MEMORY_PROPERTY_PROTECTED_BIT)           info.append("-protected");
		if (f & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) info.append("-amdcoherent");
		if (f & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) info.append("-amduncached");
	}

	return info;
}

tuple<bool, uint32_t, string> Mountain::findQueueFamilies(VkPhysicalDevice physicalDevice) {
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	vector<VkQueueFamilyProperties> queueFamilies { queueFamilyCount };
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	bool success = false;
	uint32_t selectedQueue = UINT32_MAX;
	string info = "";

	uint32_t i = 0;
	for (const auto& queueFamily : queueFamilies) {
		string familyInfo = "";

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
		familyInfo += presentSupport ? "P" : "";

		auto f = queueFamily.queueFlags;
		if (f & VK_QUEUE_GRAPHICS_BIT)       familyInfo += "G";
		if (f & VK_QUEUE_COMPUTE_BIT)        familyInfo += "C";
		if (f & VK_QUEUE_TRANSFER_BIT)       familyInfo += "T";
		if (f & VK_QUEUE_SPARSE_BINDING_BIT) familyInfo += "s";
		if (f & VK_QUEUE_PROTECTED_BIT)      familyInfo += "p";

		info.append((i > 0) ? " " : "");
		info.append(to_string(queueFamily.queueCount));
		info.append("x");
		info.append(familyInfo);

		if (presentSupport && (f & VK_QUEUE_GRAPHICS_BIT)) {
			selectedQueue = i;
			success = true;
		}
		i++;
	}

	return make_tuple(success, selectedQueue, info);
}

void Mountain::createCommandPool() {
	VkCommandPoolCreateInfo info { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	info.queueFamilyIndex = queueFamilyIndex;

	vkCreateCommandPool(device, &info, nullptr, &commandPool) >> ash("Failed to create command pool!");
}

void Mountain::createDescriptorPool() {
	array<VkDescriptorPoolSize, 2> poolSizes {};

	uint32_t size = 2 * 9000; // about ~ texture count

	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = size;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = size;

	VkDescriptorPoolCreateInfo poolInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = size;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) >> ash("Failed to create descriptor pool!");
}
