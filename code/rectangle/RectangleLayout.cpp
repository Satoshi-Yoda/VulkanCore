#include "RectangleLayout.h"

#include <cassert>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

using glm::vec2;
using glm::vec3;

using namespace std;

RectangleLayout::RectangleLayout(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater) : ash(ash), mountain(mountain), rocks(rocks), crater(crater) {
	createDescriptorSetLayout();
	createPipeline();
}

RectangleLayout::~RectangleLayout() {
	if (mountain.device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(mountain.device);

		if (descriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(mountain.device, descriptorSetLayout, nullptr);
	 	if (pipelineLayout      != VK_NULL_HANDLE) vkDestroyPipelineLayout(mountain.device, pipelineLayout, nullptr);
		if (pipeline            != VK_NULL_HANDLE) vkDestroyPipeline(mountain.device, pipeline, nullptr);
	}
}

void RectangleLayout::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uniformLayoutBinding {};
	uniformLayoutBinding.binding = 1;
	uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformLayoutBinding.descriptorCount = 1; // TODO value greater than 1 - for arrays
	uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uniformLayoutBinding.pImmutableSamplers = nullptr;

	array<VkDescriptorSetLayoutBinding, 1> bindings = { uniformLayoutBinding };

	VkDescriptorSetLayoutCreateInfo createInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();

	vkCreateDescriptorSetLayout(mountain.device, &createInfo, nullptr, &descriptorSetLayout) >> ash("Failed to create descriptor set layout!");
}

void RectangleLayout::createPipeline() {
	// TODO try catch or assert
	auto vertShaderCode = rocks.readFile("shaders/rectangle-vert.spv");
	auto fragShaderCode = rocks.readFile("shaders/rectangle-frag.spv");

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

	array<VkVertexInputBindingDescription, 1> bindingDescriptions {};
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(RectangleVertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	array<VkVertexInputAttributeDescription, 2> attributeDescriptions {};
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(RectangleVertex, pos);
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(RectangleVertex, texCoord);

	VkPipelineVertexInputStateCreateInfo vertexInputInfo { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
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
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

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
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;    // TODO We update their values through Vulkan commands, not through memory updates, and it is expected
	pipelineLayoutInfo.pPushConstantRanges = nullptr; //      that updates of push constants’ values are faster than normal memory writes.

	vkCreatePipelineLayout(mountain.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) >> ash("Failed to create pipeline layout!");

	vector<VkDynamicState> dynamicStates {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH,
	};
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipelineCreateInfo { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	pipelineCreateInfo.pTessellationState = nullptr;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pRasterizationState = &rasterizer;
	pipelineCreateInfo.pMultisampleState = &multisampling;
	pipelineCreateInfo.pDepthStencilState = nullptr;
	pipelineCreateInfo.pColorBlendState = &colorBlending;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = crater.renderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;

	vkCreateGraphicsPipelines(mountain.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) >> ash("Failed to create graphics pipeline!");

	vkDestroyShaderModule(mountain.device, fragShaderModule, nullptr);
	vkDestroyShaderModule(mountain.device, vertShaderModule, nullptr);
}
