#include "Crater.h"

#include <vector>
#include <iostream>
#include <optional>

using namespace std;

Crater::Crater(Ash &ash, Mountain &mountain, Rocks &rocks) : ash(ash), mountain(mountain), rocks(rocks) {
	init();
}

Crater::~Crater() {
	clear();
}

void Crater::reinit() {
	vkDeviceWaitIdle(mountain.device);

	clear();

	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(mountain.window, &width, &height);
		glfwWaitEvents();
	}

	init();
}

void Crater::init() {
	querySurfaceCapabilities();
	querySurfaceFormats();
	querySurfacePresentModes();

	chooseChainSize();
	chooseSurfaceFormat();
	chooseExtent();
	chooseImageUsageFlags();
	choosePresentTransform();
	choosePresentMode();

	createSwapChain();
	queryImages();
	createImageViews();
	createRenderPass();
	createFramebuffers();
}

void Crater::clear() {
	if (mountain.device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(mountain.device);
	}

	for (auto imageView : imageViews) {
		if (imageView != VK_NULL_HANDLE) vkDestroyImageView(mountain.device, imageView, nullptr);
	}

	for (auto framebuffer : framebuffers) {
		if (framebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(mountain.device, framebuffer, nullptr);
	}

	if (swapChain  != VK_NULL_HANDLE) vkDestroySwapchainKHR(mountain.device, swapChain, nullptr);
	if (renderPass != VK_NULL_HANDLE) vkDestroyRenderPass(mountain.device, renderPass, nullptr);
}

void Crater::querySurfaceCapabilities() {
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mountain.physicalDevice, mountain.surface, &capabilities);
}

void Crater::querySurfaceFormats() {
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(mountain.physicalDevice, mountain.surface, &formatCount, nullptr);
	if (formatCount != 0) {
		availableFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(mountain.physicalDevice, mountain.surface, &formatCount, availableFormats.data());
	}
}

void Crater::querySurfacePresentModes() {
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(mountain.physicalDevice, mountain.surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		availablePresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(mountain.physicalDevice, mountain.surface, &presentModeCount, availablePresentModes.data());
	}
}

void Crater::chooseChainSize() {
	chainSize = capabilities.minImageCount + ADDITIONAL_CHAIN_SIZE;
	if ((capabilities.maxImageCount > 0) && (chainSize > capabilities.maxImageCount)) {
		chainSize = capabilities.maxImageCount;
	}

	if (DISPLAY_CHAIN_SIZE) {
		printf("Swap chain length: [%d .. %s] = %d\n",
			capabilities.minImageCount,
			(capabilities.maxImageCount == 0) ? "inf" : to_string(capabilities.maxImageCount).data(),
			chainSize);
	}
}

void Crater::chooseSurfaceFormat() {
	auto preferred8bitFormat = USE_GAMMA_CORRECT ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM;

	if ((availableFormats.size() == 1) && (availableFormats[0].format == VK_FORMAT_UNDEFINED)) {
		VkSurfaceFormatKHR result { preferred8bitFormat, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		surfaceFormat = result;
	}

	optional<VkSurfaceFormatKHR> _8bit;
	optional<VkSurfaceFormatKHR> _10bit;
	for (auto format : availableFormats) {
		if (format.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			_10bit = format;
		}

		if (format.format == preferred8bitFormat && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			_8bit = format;
		}
	}

	if (USE_10_BIT && _10bit.has_value()) {
		surfaceFormat = _10bit.value();
		if (DISPLAY_BIT_DEPTH) printf("Using 10bit surface format [%d %d]\n", surfaceFormat.format, surfaceFormat.colorSpace);
	} else if (_8bit.has_value()) {
		surfaceFormat = _8bit.value();
		if (DISPLAY_BIT_DEPTH) printf("Using 8bit surface format [%d %d]\n", surfaceFormat.format, surfaceFormat.colorSpace);
	} else {
		surfaceFormat = availableFormats[0];
		if (DISPLAY_BIT_DEPTH) printf("Using first unknown surface format [%d %d]\n", surfaceFormat.format, surfaceFormat.colorSpace);
	}
}

void Crater::chooseExtent() {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		if (DISPLAY_RESOLUTION) printf("Fixed resolution %d x %d\n", capabilities.currentExtent.width, capabilities.currentExtent.height);
		extent = capabilities.currentExtent;
	} else {
		int width, height;
		glfwGetFramebufferSize(mountain.window, &width, &height);

		VkExtent2D actualExtent {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		if (DISPLAY_RESOLUTION) printf("Float resolution:   min [%d x %d]   window [%d x %d]   max [%d x %d]\n",
			capabilities.minImageExtent.width, capabilities.minImageExtent.height,
			width, height,
			capabilities.maxImageExtent.width, capabilities.maxImageExtent.height);

		actualExtent.width  = max(capabilities.minImageExtent.width,  min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, actualExtent.height));

		extent = actualExtent;
	}
}

void Crater::chooseImageUsageFlags() {
	// TODO maybe disable USAGE_TRANSFER_DST later
	if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
		imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		return;
	}
	cout << "VK_IMAGE_USAGE_TRANSFER_DST image usage is not supported by the swap chain!" << endl
		 << "Supported swap chain's image usages include:" << endl
		 << (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT             ? "    VK_IMAGE_USAGE_TRANSFER_SRC\n" : "")
		 << (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT             ? "    VK_IMAGE_USAGE_TRANSFER_DST\n" : "")
		 << (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT                  ? "    VK_IMAGE_USAGE_SAMPLED\n" : "")
		 << (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT                  ? "    VK_IMAGE_USAGE_STORAGE\n" : "")
		 << (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT         ? "    VK_IMAGE_USAGE_COLOR_ATTACHMENT\n" : "")
		 << (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ? "    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT\n" : "")
		 << (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT     ? "    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT\n" : "")
		 << (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT         ? "    VK_IMAGE_USAGE_INPUT_ATTACHMENT" : "")
		 << endl;
	imageUsageFlags = static_cast<VkImageUsageFlags>(-1);
}

void Crater::choosePresentTransform() {
	if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		if (DISPLAY_PRESENT_TRANSFORM) printf("Persent transform: VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR\n");
		presentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	} else {
		if (DISPLAY_PRESENT_TRANSFORM) printf("Persent transform: current\n");
		presentTransform = capabilities.currentTransform;
	}
}

void Crater::choosePresentMode() {
	if (DISPLAY_PRESENT_MODES) printf("Present modes:\n");
	for (auto mode : availablePresentModes) {
		if (DISPLAY_PRESENT_MODES) if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)                 printf("   immediate\n");
		if (DISPLAY_PRESENT_MODES) if (mode == VK_PRESENT_MODE_MAILBOX_KHR)                   printf("   tripple\n");
		if (DISPLAY_PRESENT_MODES) if (mode == VK_PRESENT_MODE_FIFO_KHR)                      printf("   double\n");
		if (DISPLAY_PRESENT_MODES) if (mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)              printf("   relaxed\n");
		if (DISPLAY_PRESENT_MODES) if (mode == VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR)     printf("   demand\n");
		if (DISPLAY_PRESENT_MODES) if (mode == VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR) printf("   continuous\n");
	}
	if (DISPLAY_PRESENT_MODES) printf("\n");
	// TODO actually check if these are supported
	presentMode = USE_VSYNC ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
	// presentMode = VK_PRESENT_MODE_FIFO_KHR;
}

void Crater::createSwapChain() {
	VkSwapchainCreateInfoKHR createInfo { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	createInfo.surface = mountain.surface;
	createInfo.minImageCount = chainSize;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = imageUsageFlags;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = nullptr;
	createInfo.preTransform = presentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	vkCreateSwapchainKHR(mountain.device, &createInfo, nullptr, &swapChain) >> ash("Failed to create swap chain!");
}

void Crater::queryImages() {
	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(mountain.device, swapChain, &imageCount, nullptr) >> ash("Failed to get count of swap chain images!");
	(imageCount != 0)                                                         >> ash("Zero images in swap chain!");

	images.resize(imageCount);
	vkGetSwapchainImagesKHR(mountain.device, swapChain, &imageCount, images.data()) >> ash("Failed to get swap chain images!");

	if (DISPLAY_CHAIN_SIZE) printf("Swap chain final length: %d\n", imageCount);
}

void Crater::createImageViews() {
	imageViews.resize(images.size());

	for (size_t i = 0; i < images.size(); i++) {
		imageViews[i] = rocks.createImageView(images[i], surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}

void Crater::createRenderPass() {
	VkAttachmentDescription colorAttachment {};
	colorAttachment.format = surfaceFormat.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	vector<VkAttachmentDescription> attachments { colorAttachment };

	VkAttachmentReference colorAttachmentRef {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = nullptr;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	// TODO what does that dependencies do? (no visible effect so far)
	VkSubpassDependency dependency1 {};
	dependency1.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency1.dstSubpass = 0;
	dependency1.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency1.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency1.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependency1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency1.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkSubpassDependency dependency2 {};
	dependency2.srcSubpass = 0;
	dependency2.dstSubpass = VK_SUBPASS_EXTERNAL;
	dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency2.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependency2.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	vector<VkSubpassDependency> dependencies { dependency1, dependency2 };

	VkRenderPassCreateInfo createInfo { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	createInfo.pAttachments = attachments.data();
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;
	createInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	createInfo.pDependencies = dependencies.data();

	vkCreateRenderPass(mountain.device, &createInfo, nullptr, &renderPass) >> ash("Failed to create render pass!");
}

void Crater::createFramebuffers() {
	framebuffers.resize(images.size());

	for (size_t i = 0; i < images.size(); i++) {
		VkFramebufferCreateInfo framebufferCreateInfo { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebufferCreateInfo.renderPass = renderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = &imageViews[i];
		framebufferCreateInfo.width  = extent.width;
		framebufferCreateInfo.height = extent.height;
		framebufferCreateInfo.layers = 1;

		vkCreateFramebuffer(mountain.device, &framebufferCreateInfo, nullptr, &framebuffers[i]) >> ash("Failed to create framebuffer!");
	}
}
