#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

using namespace std;
// using namespace glm;

const int MAX_FRAMES_IN_FLIGHT = 2;

const bool USE_GAMMA_CORRECT = false;
const bool USE_10_BIT = false;

const bool DISPLAY_LAYERS              = false;
const bool DISPLAY_INSTANCE_EXTENSIONS = false;
const bool DISPLAY_DEVICE_EXTENSIONS   = false;
const bool DISPLAY_DEVICES             = false;
const bool DISPLAY_BIT_DEPTH           = false;
const bool DISPLAY_PRESENT_MODES       = false;
const bool DISPLAY_SWAP_RESOLUTION     = false;
const bool DISPLAY_SWAP_LENGTH         = false;

const bool USE_VALIDATION_LAYERS = true;
const vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_KHRONOS_validation",
};
const vector<const char*> DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};

const vector<Vertex> vertices = {
	{{ 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
	{{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

class HelloTriangleApplication {
public:
	void run() {
		startLoading = chrono::high_resolution_clock::now();
		initWindow();
		initVulkan();
		finishLoading = chrono::high_resolution_clock::now();
		lastWindowTitleUpdate = startLoading;
		auto deltaLoading = chrono::duration_cast<chrono::duration<double>>(finishLoading - startLoading).count();
		printf("Initialized in %.2fs\n", deltaLoading);
		mainLoop();
		cleanup();
	}

private:
	struct QueueFamilyIndices {
		optional<uint32_t> graphicsFamily;
		optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		vector<VkSurfaceFormatKHR> formats;
		vector<VkPresentModeKHR> presentModes;
	};

	chrono::time_point<chrono::high_resolution_clock> startLoading;
	chrono::time_point<chrono::high_resolution_clock> finishLoading;
	chrono::time_point<chrono::high_resolution_clock> lastWindowTitleUpdate;

	uint32_t windowWidth = 1200;
	uint32_t windowHeight = 900;
	GLFWwindow* window;
	VkSurfaceKHR surface;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	// TODO
	// In tutorial code that 2 fields was calculated at the need.
	// I stored them in fields. Later figured out that I need to get new 
	// SwapChainSupportDetails after window resized.
	// So, check that again.
	QueueFamilyIndices familyIndices;
	SwapChainSupportDetails swapChainSupport;
	
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSwapchainKHR swapChain;
	vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	vector<VkImageView> swapChainImageViews;
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	vector<VkFramebuffer> swapChainFramebuffers;
	VkCommandPool commandPool;
	vector<VkCommandBuffer> commandBuffers;
	vector<VkSemaphore> imageAvailableSemaphores;
	vector<VkSemaphore> renderFinishedSemaphores;
	vector<VkFence> inFlightFences;
	vector<VkFence> imagesInFlight;
	size_t currentFrame = 0;
	bool framebufferResized = false;
	uint64_t frame = 0;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	void initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		window = glfwCreateWindow(windowWidth, windowHeight, "Hello Triangle", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	void initVulkan() {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createCommandBuffers();
		createVertexBuffer();
		createSyncObjects();
	}

	void recreateSwapChain() {
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device);

		cleanupSwapChain();

		// TODO
		// It is possible to create a new swap chain while drawing commands on an image from the old swap chain are still in-flight.
		// You need to pass the previous swap chain to the oldSwapChain field in the VkSwapchainCreateInfoKHR struct and destroy
		// the old swap chain as soon as you've finished using it.

		swapChainSupport = querySwapChainSupport(physicalDevice);

		createSwapChain();
		createImageViews();
		createRenderPass(); // TODO it is rare for the swap chain image format to change during window resize
		createGraphicsPipeline(); // TODO can be avoided with using dynamic state for the viewports and scissor rectangles
		createFramebuffers();
		createCommandBuffers();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			drawFrame();
		}
		vkDeviceWaitIdle(device);
	}

	void drawFrame() {
		frame++;

		auto now = chrono::high_resolution_clock::now();
		auto afterWindowUpdate = chrono::duration_cast<chrono::duration<double>>(now - lastWindowTitleUpdate).count();
		if (afterWindowUpdate > 0.1) {
			lastWindowTitleUpdate = now;
			auto runtime = chrono::duration_cast<chrono::duration<double>>(now - finishLoading).count();
			double fps = frame / runtime;
			char str[100];
			sprintf(str, "Hello Triangle   size %d x %d   frame %lld   fps %.0f", windowWidth, windowHeight, frame, fps);
			glfwSetWindowTitle(window, str);
		}

		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw runtime_error("Failed to acquire swap chain image!");
		}

		// Check if a previous frame is using this image (i.e. there is its fence to wait on)
		if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		}
		// Mark the image as now being in use by this frame
		imagesInFlight[imageIndex] = inFlightFences[currentFrame];

		VkSubmitInfo submitInfo {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(device, 1, &inFlightFences[currentFrame]);

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw runtime_error("Failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		} else if (result != VK_SUCCESS) {
			throw runtime_error("Failed to present swap chain image!");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		// vkQueueWaitIdle(presentQueue); // TODO maybe once more check it without MAX_FRAMES_IN_FLIGHT feature
	}

	void createVertexBuffer() {
		VkBufferCreateInfo bufferInfo {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(vertices[0]) * vertices.size();
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferInfo.flags = 0;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
			throw runtime_error("failed to create vertex buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
			throw runtime_error("Failed to allocate vertex buffer memory!");
		}

		vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

		void* data;
		vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data); // It is also possible to specify the special value VK_WHOLE_SIZE to map all of the memory.
		memcpy(data, vertices.data(), (size_t) bufferInfo.size);
		vkUnmapMemory(device, vertexBufferMemory);
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		throw runtime_error("Failed to find suitable memory type!");
	}

	void createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			{
				throw runtime_error("Failed to create synchronization objects for a frame!");
			}
		}
	}

	void createCommandBuffers() {
		commandBuffers.resize(swapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocInfo {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();
		
		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw runtime_error("Failed to allocate command buffers!");
		}

		for (size_t i = 0; i < commandBuffers.size(); i++) {
			VkCommandBufferBeginInfo beginInfo {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;
			beginInfo.pInheritanceInfo = nullptr;

			if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
				throw runtime_error("Failed to begin recording command buffer!");
			}

			VkRenderPassBeginInfo renderPassInfo {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = swapChainExtent;
			VkClearValue clearColor = { 0.25f, 0.25f, 0.25f, 1.0f };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
			VkBuffer vertexBuffers[] = { vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);
			vkCmdEndRenderPass(commandBuffers[i]);

			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw runtime_error("Failed to record command buffer!");
			}
		}
	}

	void createCommandPool() {
		VkCommandPoolCreateInfo poolInfo {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = familyIndices.graphicsFamily.value();
		poolInfo.flags = 0;

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw runtime_error("Failed to create command pool!");
		}
	}

	void createFramebuffers() {
		swapChainFramebuffers.resize(swapChainImageViews.size());

		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			VkImageView attachments[] = { swapChainImageViews[i] };

			VkFramebufferCreateInfo framebufferInfo {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw runtime_error("Failed to create framebuffer!");
			}
		}
	}

	void createRenderPass() {
		VkSubpassDependency dependency {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkAttachmentDescription colorAttachment {};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkRenderPassCreateInfo renderPassInfo {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw runtime_error("Failed to create render pass!");
		}
	}

	void createGraphicsPipeline() {
		// TODO try catch
		auto vertShaderCode = readFile("shaders/shader.vert.spv");
		auto fragShaderCode = readFile("shaders/shader.frag.spv");

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";
		// TODO pSpecializationInfo can contain custom flags that can help compiler discarg if-s inside shader code during compiling spv -> assembler
		vertShaderStageInfo.pSpecializationInfo = nullptr;

		VkPipelineShaderStageCreateInfo fragShaderStageInfo {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		// vertexInputInfo.vertexBindingDescriptionCount = 0;
		// vertexInputInfo.pVertexBindingDescriptions = nullptr;
		// vertexInputInfo.vertexAttributeDescriptionCount = 0;
		// vertexInputInfo.pVertexAttributeDescriptions = nullptr;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float) swapChainExtent.width;
		viewport.height = (float) swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor {};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		// TODO use dynamic states or comment them
		// not used for now...
		// VkDynamicState dynamicStates[] = {
		// 	VK_DYNAMIC_STATE_VIEWPORT,
		// 	VK_DYNAMIC_STATE_LINE_WIDTH,
		// };
		// VkPipelineDynamicStateCreateInfo dynamicState {};
		// dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		// dynamicState.dynamicStateCount = 2;
		// dynamicState.pDynamicStates = dynamicStates;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw runtime_error("Failed to create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw runtime_error("Failed to create graphics pipeline!");
		}

		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}

	VkShaderModule createShaderModule(const vector<char>& code) {
		VkShaderModuleCreateInfo createInfo {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw runtime_error("Failed to create shader module!");
		}
		return shaderModule;
	}

	static vector<char> readFile(const string& filename) {
		ifstream file(filename, ios::ate | ios::binary);
		if (!file.is_open()) {
			throw runtime_error("Failed to open file!");
		}

		size_t fileSize = (size_t) file.tellg();
		vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	void createImageViews() {
		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw runtime_error("Failed to create image views!");
			}
		}
	}

	void createSwapChain() {
		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		swapChainExtent = chooseSwapExtent(swapChainSupport.capabilities);
		uint32_t imageCount = chooseSwapImageCount(swapChainSupport.capabilities);
		windowWidth = swapChainExtent.width;
		windowHeight = swapChainExtent.height;

		swapChainImageFormat = surfaceFormat.format;

		VkSwapchainCreateInfoKHR createInfo {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = swapChainExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t queueFamilyIndices[] = { familyIndices.graphicsFamily.value(), familyIndices.presentFamily.value() };
		if (familyIndices.graphicsFamily != familyIndices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw runtime_error("Failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		if (DISPLAY_SWAP_LENGTH) printf("Final swap chain length: %d\n", imageCount);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR>& availableFormats) {
		optional<VkSurfaceFormatKHR> _8bit;
		optional<VkSurfaceFormatKHR> _10bit;
		for (auto format : availableFormats) {
			if (format.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				_10bit = format;
			}

			auto preferred8bitFormat = USE_GAMMA_CORRECT ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM;
			if (format.format == preferred8bitFormat && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				_8bit = format;
			}
		}

		if (USE_10_BIT && _10bit.has_value()) {
			if (DISPLAY_BIT_DEPTH) printf("Using 10bit surface format\n");
			return _10bit.value();
		} else if (_8bit.has_value()) {
			if (DISPLAY_BIT_DEPTH) printf("Using 8bit surface format\n");
			return _8bit.value();
		} else {
			if (DISPLAY_BIT_DEPTH) printf("Using largest unknown surface format\n");
			return availableFormats[0];
		}
	}

	VkPresentModeKHR chooseSwapPresentMode(const vector<VkPresentModeKHR>& availablePresentModes) {
		if (DISPLAY_PRESENT_MODES) printf("Present modes:\n");
		for (auto mode : availablePresentModes) {
			if (DISPLAY_PRESENT_MODES) if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)                  printf("   immediate\n");
			if (DISPLAY_PRESENT_MODES) if (mode == VK_PRESENT_MODE_MAILBOX_KHR )                   printf("   tripple\n");
			if (DISPLAY_PRESENT_MODES) if (mode == VK_PRESENT_MODE_FIFO_KHR )                      printf("   double\n");
			if (DISPLAY_PRESENT_MODES) if (mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR )              printf("   relaxed\n");
			if (DISPLAY_PRESENT_MODES) if (mode == VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR )     printf("   demand\n");
			if (DISPLAY_PRESENT_MODES) if (mode == VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR ) printf("   continuous\n");
		}
		if (DISPLAY_PRESENT_MODES) printf("\n");
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != UINT32_MAX) {
			if (DISPLAY_SWAP_RESOLUTION) printf("Fixed swap resolution %d x %d\n", capabilities.currentExtent.width, capabilities.currentExtent.height);
			return capabilities.currentExtent;
		} else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			if (DISPLAY_SWAP_RESOLUTION) printf("Float swap resolution:   min [%d x %d]   window [%d x %d]   max [%d x %d]\n",
				capabilities.minImageExtent.width, capabilities.minImageExtent.height,
				width, height,
				capabilities.maxImageExtent.width, capabilities.maxImageExtent.height);

			actualExtent.width  = max(capabilities.minImageExtent.width,  min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	uint32_t chooseSwapImageCount(const VkSurfaceCapabilitiesKHR& capabilities) {
		uint32_t result = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && result > swapChainSupport.capabilities.maxImageCount) {
			result = swapChainSupport.capabilities.maxImageCount;
		}

		if (DISPLAY_SWAP_LENGTH) printf("Swap queue length: [%d .. %s] = %d\n",
			swapChainSupport.capabilities.minImageCount,
			(swapChainSupport.capabilities.maxImageCount == 0) ? "inf" : to_string(swapChainSupport.capabilities.maxImageCount).data(),
			result);

		return result;
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice) {
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	void createSurface() {
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw runtime_error("failed to create window surface!");
		}
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, string &familiesInfo) {
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		familiesInfo = "";

		int i = 0;
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

			familiesInfo.append((i > 0) ? " " : "");
			familiesInfo.append(to_string(queueFamily.queueCount));
			familiesInfo.append("x");
			familiesInfo.append(familyInfo);

			// TODO find the family that supports P and G, if it is possible, and use it
			// TODO test for performance when P and G deliberately selected as different families
			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (f & VK_QUEUE_GRAPHICS_BIT) {
 				indices.graphicsFamily = i;
			}
			i++;
		}

		return indices;
	}

	void createLogicalDevice() {
		vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		set<uint32_t> uniqueQueueFamilies = { familyIndices.graphicsFamily.value(), familyIndices.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures {};

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
		createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
		if (USE_VALIDATION_LAYERS) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw runtime_error("Failed to create logical device!");
		}

		vkGetDeviceQueue(device, familyIndices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, familyIndices.presentFamily.value(), 0, &presentQueue);
	}

	void pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0) {
			throw runtime_error("Failed to find GPUs with Vulkan support!");
		}

		vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		if (DISPLAY_DEVICES) printf("Devices:\n");
		for (const auto& device : devices) {
			if (isDeviceSuitable(device)) {
				if (physicalDevice == VK_NULL_HANDLE) {
					physicalDevice = device;
				}
			}
		}
		if (DISPLAY_DEVICES) printf("\n");

		if (physicalDevice == VK_NULL_HANDLE) {
			throw runtime_error("Failed to find a suitable GPU!");
		}
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
		vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

		set<string> required(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

		if (DISPLAY_DEVICE_EXTENSIONS) printf("Device extensions:\n");
		for (auto extension : availableExtensions) {
			if (required.find(extension.extensionName) != required.end()) {
				required.erase(extension.extensionName);
				if (DISPLAY_DEVICE_EXTENSIONS) printf(" + %s\n", extension);
			} else {
				if (DISPLAY_DEVICE_EXTENSIONS) printf("   %s\n", extension);
			}
		}
		if (DISPLAY_DEVICE_EXTENSIONS) printf("\n");

		return required.empty();
	}

	bool isDeviceSuitable(VkPhysicalDevice physicalDevice) {
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

		VkPhysicalDeviceMemoryProperties deviceMemory;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemory);
		vector<VkMemoryType> memoryTypes { deviceMemory.memoryTypes, deviceMemory.memoryTypes + deviceMemory.memoryTypeCount };
		vector<VkMemoryHeap> memoryHeaps { deviceMemory.memoryHeaps, deviceMemory.memoryHeaps + deviceMemory.memoryHeapCount };

		string typeInfo = "";
		for (auto type : memoryTypes) {
			typeInfo.append((typeInfo != "") ? " " : "");
			typeInfo.append(to_string(type.heapIndex));
			auto f = type.propertyFlags;
			if (f & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)        typeInfo.append("-local");
			if (f & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)        typeInfo.append("-hostvisible");
			if (f & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)       typeInfo.append("-hostcoherent");
			if (f & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)         typeInfo.append("-hostcached");
			if (f & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)    typeInfo.append("-lazily");
			if (f & VK_MEMORY_PROPERTY_PROTECTED_BIT)           typeInfo.append("-protected");
			if (f & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) typeInfo.append("-amdcoherent");
			if (f & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) typeInfo.append("-amduncached");
		}

		string heapInfo = "";
		for (auto heap : memoryHeaps) {
			heapInfo.append((heapInfo != "") ? " " : "");
			char str[10];
			sprintf(str, "%.2fGB", (double)heap.size / (1 << 30));
			heapInfo.append(str);
		}

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

		string familiesInfo { "none" };
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice, familiesInfo);

		bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

		bool swapChainAdequate = false;
		if (extensionsSupported) {
			swapChainSupport = querySwapChainSupport(physicalDevice);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		bool result =
			indices.isComplete() &&
			extensionsSupported &&
			swapChainAdequate;

		if (DISPLAY_DEVICES) printf(" %s %s [%s] [%s] [%s]\n", (result ? "+" : " "), deviceProperties.deviceName, familiesInfo.data(), heapInfo.data(), typeInfo.data());

		if (result) {
			familyIndices = indices;
		}

		return result;
	}

	void createInstance() {
		if (USE_VALIDATION_LAYERS && !checkValidationLayerSupport()) {
			throw runtime_error("ERROR Validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 3);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		auto requiredExtensions = getRequiredExtensions();
		if (!checkRequiredExtensionsSupport(requiredExtensions)) {
			throw runtime_error("ERROR Required extensions not found!");
		}

		VkInstanceCreateInfo createInfo {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();
		createInfo.enabledLayerCount = 0;

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (USE_VALIDATION_LAYERS) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
		} else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw runtime_error("ERROR Failed to create instance!");
		}
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
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

	void setupDebugMessenger() {
		if (!USE_VALIDATION_LAYERS) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw runtime_error("Failed to set up debug messenger!");
		}
	}

	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		set<string> required;
		for (auto layer : VALIDATION_LAYERS) {
			required.insert(layer);
		}

		if (DISPLAY_LAYERS) printf("\nValidation layers:\n");
		set<string> available;
		for (auto layer : availableLayers) {
			available.insert(layer.layerName);
			if (required.find(layer.layerName) != required.end()) {
				if (DISPLAY_LAYERS) printf(" + %s\n", layer);
			} else {
				if (DISPLAY_LAYERS) printf("   %s\n", layer);
			}
		}
		if (DISPLAY_LAYERS) printf("\n");

		bool result = true;
		for (auto layer : required) {
			if (available.find(layer) == available.end()) {
				printf("ERROR Validation layer %s not available!\n", layer);
				result = false;
			}
		}
		return result;
	}

	vector<const char*> getRequiredExtensions() {
		uint32_t requiredExtensionCount = 0;
		const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);

		vector<const char*> result(requiredExtensions, requiredExtensions + requiredExtensionCount);

		if (USE_VALIDATION_LAYERS) {
		    result.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return result;
	}

	bool checkRequiredExtensionsSupport(vector<const char*> &requiredExtensions) {
		set<string> required {};
		for (int i = 0; i < requiredExtensions.size(); i++) {
			required.insert(requiredExtensions[i]);
		}

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		set<string> supported;

		if (DISPLAY_INSTANCE_EXTENSIONS) printf("Instance extensions:\n");
		for (auto extension : extensions) {
			supported.insert(extension.extensionName);
			if (required.find(extension.extensionName) != required.end()) {
				if (DISPLAY_INSTANCE_EXTENSIONS) printf(" + %s\n", extension);
			} else {
				if (DISPLAY_INSTANCE_EXTENSIONS) printf("   %s\n", extension);
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

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			string severity = "[NONE]";
			switch (messageSeverity) {
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
					severity = "[INFO]";
					break;
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
					severity = "[VERBOSE]";
					break;
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
					severity = "[WARNING]";
					break;
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
					severity = "[ERROR]";
					break;
			}
			cerr << severity << " Validation layer: " << pCallbackData->pMessage << endl;
		}
		return VK_FALSE;
	}

	void cleanupSwapChain() {
		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}
		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);
		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}
		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}

	void cleanup() {
		cleanupSwapChain();

		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}
		vkDestroyCommandPool(device, commandPool, nullptr);
		vkDestroyDevice(device, nullptr);
		if (USE_VALIDATION_LAYERS) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}
};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	} catch (const exception& e) {
		cerr << e.what() << endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
