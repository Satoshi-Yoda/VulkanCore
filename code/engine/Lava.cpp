#include "Lava.h"
#include "../utils/Loader.h"

#include <vector>
#include <array>

using namespace std;

VkVertexInputBindingDescription Vertex::getBindingDescription() {
	VkVertexInputBindingDescription bindingDescription {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions() {
	array<VkVertexInputAttributeDescription, 3> attributeDescriptions {};

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

	return attributeDescriptions;
}

Lava::Lava(Ash &ash, Mountain &mountain, Rocks &rocks, Crater &crater) : ash(ash), mountain(mountain), rocks(rocks), crater(crater) {
	createTextureSampler();

	createDescriptorSetLayout();
	createDescriptorSetLayout2();

	createRenderPass();
	createPipeline();
}

Lava::~Lava() {
	if (mountain.device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(mountain.device);
	}

	if (vertexBuffer         != VK_NULL_HANDLE) vkDestroyBuffer(mountain.device, vertexBuffer, nullptr);
	if (vertexBufferMemory   != VK_NULL_HANDLE) vkFreeMemory(mountain.device, vertexBufferMemory, nullptr);
 
	if (textureSampler       != VK_NULL_HANDLE) vkDestroySampler(mountain.device, textureSampler, nullptr);
	if (textureImageView     != VK_NULL_HANDLE) vkDestroyImageView(mountain.device, textureImageView, nullptr);
	if (textureImage         != VK_NULL_HANDLE) vkDestroyImage(mountain.device, textureImage, nullptr);
	if (textureImageMemory   != VK_NULL_HANDLE) vkFreeMemory(mountain.device, textureImageMemory, nullptr);

	if (descriptorSetLayout  != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(mountain.device, descriptorSetLayout, nullptr);
	if (descriptorSetLayout2 != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(mountain.device, descriptorSetLayout2, nullptr);
 
	if (pipelineLayout       != VK_NULL_HANDLE) vkDestroyPipelineLayout(mountain.device, pipelineLayout, nullptr);
	if (pipeline             != VK_NULL_HANDLE) vkDestroyPipeline(mountain.device, pipeline, nullptr);
	if (renderPass           != VK_NULL_HANDLE) vkDestroyRenderPass(mountain.device, renderPass, nullptr);
}

void Lava::createRenderPass() {
	VkAttachmentDescription colorAttachment {};
	colorAttachment.format = crater.surfaceFormat.format;
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

void Lava::recreatePipeline() {
	if (pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(mountain.device, pipelineLayout, nullptr);
	if (pipeline       != VK_NULL_HANDLE) vkDestroyPipeline(mountain.device, pipeline, nullptr);
	createPipeline();
}

void Lava::createPipeline() {
	// TODO try catch
	auto vertShaderCode = rocks.readFile("shaders/shader.vert.spv");
	auto fragShaderCode = rocks.readFile("shaders/shader.frag.spv");

	VkShaderModule vertShaderModule = rocks.createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = rocks.createShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	// TODO pSpecializationInfo can contain custom flags that can help compiler discarg if-s inside shader code during compiling spv -> assembler
	vertShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo fragShaderStageInfo { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	vector<VkPipelineShaderStageCreateInfo> shaderStages { vertShaderStageInfo, fragShaderStageInfo };

	auto bindingDescription    = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // TODO move this to setup VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
	inputAssembly.primitiveRestartEnable = VK_FALSE; // TODO parameter that tells whether a special index value (when indexed drawing is performed) restarts assembly of a given primitive

	VkViewport viewport {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width  = static_cast<float>(crater.extent.width);
	viewport.height = static_cast<float>(crater.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor {};
	scissor.offset = { 0, 0 };
	scissor.extent = crater.extent;

	VkPipelineViewportStateCreateInfo viewportState { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
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

	VkPipelineMultisampleStateCreateInfo multisampling { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.minSampleShading = 1.0f;

	VkPipelineColorBlendAttachmentState colorBlendAttachment {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout2;
	pipelineLayoutInfo.pushConstantRangeCount = 0;    // TODO We update their values through Vulkan commands, not through memory updates, and it is expected
	pipelineLayoutInfo.pPushConstantRanges = nullptr; //      that updates of push constants’ values are faster than normal memory writes.

	vkCreatePipelineLayout(mountain.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) >> ash("Failed to create pipeline layout!");

	// TODO use dynamic states or comment them
	// not used for now...
	// VkDynamicState dynamicStates[] {
	// 	VK_DYNAMIC_STATE_VIEWPORT,
	// 	VK_DYNAMIC_STATE_LINE_WIDTH,
	// };
	// VkPipelineDynamicStateCreateInfo dynamicState { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	// dynamicState.dynamicStateCount = 2;
	// dynamicState.pDynamicStates = dynamicStates;

	VkGraphicsPipelineCreateInfo pipelineInfo { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pTessellationState = nullptr;
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

	vkCreateGraphicsPipelines(mountain.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) >> ash("Failed to create graphics pipeline!");

	vkDestroyShaderModule(mountain.device, fragShaderModule, nullptr);
	vkDestroyShaderModule(mountain.device, vertShaderModule, nullptr);
}

void Lava::establishVertexBuffer(vector<Vertex> vertices) {
	vertexBufferSize = vertices.size();
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

	// TODO try keep staging buffers and reuse them later
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	rocks.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	rocks.copyDataToBuffer(vertices.data(), stagingBufferMemory, static_cast<size_t>(bufferSize));
	rocks.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
	rocks.copyBufferToBuffer(stagingBuffer, vertexBuffer, bufferSize, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);

	vkDestroyBuffer(mountain.device, stagingBuffer, nullptr);
	vkFreeMemory(mountain.device, stagingBufferMemory, nullptr);
}

void Lava::establishTexture(int width, int height, void* pixels) {
	int mipLevels = 1;
	// mipLevels = static_cast<uint32_t>(floor(log2(max(width, height)))) + 1;
	VkDeviceSize imageSize = width * height * 4;
	(pixels != nullptr) >> ash("Failed to load texture image!");

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	rocks.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	rocks.copyDataToBuffer(pixels, stagingBufferMemory, imageSize);

	auto preferred8bitFormat = crater.USE_GAMMA_CORRECT ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

	rocks.createImage(static_cast<uint32_t>(width), static_cast<uint32_t>(height), mipLevels, VK_SAMPLE_COUNT_1_BIT, preferred8bitFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		textureImage, textureImageMemory);

	// TODO use common command buffer for this operations to ensure performance
	rocks.transitionImageLayout(textureImage, preferred8bitFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels); // TODO check why in tutorial no mipLevels here
	rocks.copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	rocks.transitionImageLayout(textureImage, preferred8bitFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);
	// generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, width, height, mipLevels);

	vkDestroyBuffer(mountain.device, stagingBuffer, nullptr);
	vkFreeMemory(mountain.device, stagingBufferMemory, nullptr);

	textureImageView = rocks.createImageView(textureImage, preferred8bitFormat, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
}

void Lava::createTextureSampler() {
	VkSamplerCreateInfo samplerInfo { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE; // TODO with true it is possible to use [0..width) instead of [0..1) !!
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(mipLevels);
	samplerInfo.mipLodBias = 0.0f;

	vkCreateSampler(mountain.device, &samplerInfo, nullptr, &textureSampler) >> ash("Failed to create texture sampler!");
}

// TODO remove or move to scene
void Lava::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding samplerLayoutBinding {};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo createInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	createInfo.bindingCount = 1;
	createInfo.pBindings = &samplerLayoutBinding;

	vkCreateDescriptorSetLayout(mountain.device, &createInfo, nullptr, &descriptorSetLayout) >> ash("Failed to create descriptor set layout!");
}

void Lava::createDescriptorSetLayout2() {
	VkDescriptorSetLayoutBinding samplerLayoutBinding {};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding uniformLayoutBinding {};
	uniformLayoutBinding.binding = 1;
	uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformLayoutBinding.descriptorCount = 1; // TODO value greater than 1 - for arrays
	uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uniformLayoutBinding.pImmutableSamplers = nullptr;

	array<VkDescriptorSetLayoutBinding, 2> bindings = { samplerLayoutBinding, uniformLayoutBinding };

	VkDescriptorSetLayoutCreateInfo createInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();

	vkCreateDescriptorSetLayout(mountain.device, &createInfo, nullptr, &descriptorSetLayout2) >> ash("Failed to create descriptor set layout!");
}
