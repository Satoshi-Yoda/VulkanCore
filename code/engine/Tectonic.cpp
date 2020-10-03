#include "Tectonic.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <array>
#include <chrono>

using namespace std;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::radians;

struct UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
};

Tectonic::Tectonic(Ash &ash, Mountain &mountain, Rocks &rocks, Crater &crater, Lava &lava) : ash(ash), mountain(mountain), rocks(rocks), crater(crater), lava(lava) {
	createInFlightResources();
	createUniformBuffers();
}

Tectonic::~Tectonic() {
	if (mountain.device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(mountain.device);
	}

	for (uint32_t i = 0; i < IN_FLIGHT_FRAMES; i++) {
		if (commandBuffersArray[i]      != VK_NULL_HANDLE) vkFreeCommandBuffers(mountain.device, mountain.commandPool, 1, &commandBuffersArray[i]);
		if (imageAvailableSemaphores[i] != VK_NULL_HANDLE) vkDestroySemaphore  (mountain.device, imageAvailableSemaphores[i], nullptr);
		if (renderFinishedSemaphores[i] != VK_NULL_HANDLE) vkDestroySemaphore  (mountain.device, renderFinishedSemaphores[i], nullptr);
		if (fences[i]                   != VK_NULL_HANDLE) vkDestroyFence      (mountain.device, fences[i], nullptr);
		if (framebuffers[i]             != VK_NULL_HANDLE) vkDestroyFramebuffer(mountain.device, framebuffers[i], nullptr);
		if (uniformBuffers[i]           != VK_NULL_HANDLE) vkDestroyBuffer     (mountain.device, uniformBuffers[i], nullptr);
		if (uniformBuffersMemory[i]     != VK_NULL_HANDLE) vkFreeMemory        (mountain.device, uniformBuffersMemory[i], nullptr);
	}
}

void Tectonic::createInFlightResources() {
	VkCommandBufferAllocateInfo allocateInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandPool = mountain.commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = IN_FLIGHT_FRAMES;

	vkAllocateCommandBuffers(mountain.device, &allocateInfo, commandBuffersArray.data()) >> ash("Failed to allocate command buffers!");

	for (int i = 0; i < IN_FLIGHT_FRAMES; i++) {
		VkSemaphoreCreateInfo semaphore_create_info { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		vkCreateSemaphore(mountain.device, &semaphore_create_info, nullptr, &imageAvailableSemaphores[i]) >> ash("Failed to create semaphore!");
		vkCreateSemaphore(mountain.device, &semaphore_create_info, nullptr, &renderFinishedSemaphores[i]) >> ash("Failed to create semaphore!");

		VkFenceCreateInfo fenceInfo { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		vkCreateFence(mountain.device, &fenceInfo, nullptr, &fences[i]) >> ash("Failed to create fence!");

		framebuffers[i] = VK_NULL_HANDLE;
	}
}

void Tectonic::createUniformBuffers() {
	for (size_t i = 0; i < IN_FLIGHT_FRAMES; i++) {
		rocks.createBuffer(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
	}
}

void Tectonic::createDescriptorSets() {
	vector<VkDescriptorSetLayout> layouts(IN_FLIGHT_FRAMES, lava.descriptorSetLayout2);
	VkDescriptorSetAllocateInfo allocInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.descriptorPool = mountain.descriptorPool;
	allocInfo.descriptorSetCount = IN_FLIGHT_FRAMES;
	allocInfo.pSetLayouts = layouts.data();

	vkAllocateDescriptorSets(mountain.device, &allocInfo, descriptorSets.data()) >> ash("Failed to allocate tectonic descriptor set!");

	for (size_t i = 0; i < IN_FLIGHT_FRAMES; i++) {
		VkDescriptorImageInfo imageInfo {};
		imageInfo.sampler = lava.textureSampler;
		imageInfo.imageView = lava.textureImageView;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkDescriptorBufferInfo bufferInfo {};
		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet imageWrite { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		imageWrite.dstSet = descriptorSets[i];
		imageWrite.dstBinding = 0;
		imageWrite.dstArrayElement = 0;
		imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageWrite.descriptorCount = 1;
		imageWrite.pImageInfo = &imageInfo;
		
		VkWriteDescriptorSet bufferWrite { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		bufferWrite.dstSet = descriptorSets[i];
		bufferWrite.dstBinding = 1;
		bufferWrite.dstArrayElement = 0;
		bufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bufferWrite.descriptorCount = 1;
		bufferWrite.pBufferInfo = &bufferInfo;

		array<VkWriteDescriptorSet, 2> descriptorWrites { imageWrite, bufferWrite };

		vkUpdateDescriptorSets(mountain.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Tectonic::updateUniformBuffer() {
	static auto startTime = chrono::high_resolution_clock::now();
	static auto  lastTime = chrono::high_resolution_clock::now();
	auto      currentTime = chrono::high_resolution_clock::now();
	float time = chrono::duration<float, chrono::seconds::period>(currentTime - startTime).count();
	float dt = chrono::duration<float, chrono::seconds::period>(currentTime - lastTime).count();
	lastTime = currentTime;

	UniformBufferObject ubo {};
	ubo.model = glm::rotate(mat4(1.0f), 0.5f * time * radians(90.0f), vec3(0.0f, 0.0f, 1.0f));
	ubo.view  = glm::lookAt(vec3(2.0f, 2.0f, 2.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f));
	ubo.proj  = glm::perspective(radians(45.0f), (float)crater.extent.width / (float)crater.extent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	// TODO map only once, and keep it mapped, better for performance
	void* data;
	vkMapMemory(mountain.device, uniformBuffersMemory[inFlightIndex], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(mountain.device, uniformBuffersMemory[inFlightIndex]);
}

void Tectonic::prepareFrame(uint32_t craterIndex) {
	if (framebuffers[inFlightIndex] != VK_NULL_HANDLE) {
		vkDestroyFramebuffer(mountain.device, framebuffers[inFlightIndex], nullptr);
	}

	// TODO
	// I can draw to separate single image with commandBuffer with a lot of operations,
	// and then copy it to dedicated swapChain image with prepared short commandBuffer

	VkFramebufferCreateInfo framebufferCreateInfo { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferCreateInfo.renderPass = lava.renderPass;
	framebufferCreateInfo.attachmentCount = 1;
	framebufferCreateInfo.pAttachments = &crater.imageViews[craterIndex];
	framebufferCreateInfo.width  = crater.extent.width;
	framebufferCreateInfo.height = crater.extent.height;
	framebufferCreateInfo.layers = 1;

	vkCreateFramebuffer(mountain.device, &framebufferCreateInfo, nullptr, &framebuffers[inFlightIndex]) >> ash("Failed to create framebuffer!");

	VkCommandBufferBeginInfo bufferBeginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	bufferBeginInfo.pInheritanceInfo = nullptr;

	VkImageSubresourceRange imageSubresourceRange {};
	imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageSubresourceRange.baseMipLevel = 0;
	imageSubresourceRange.levelCount = 1;
	imageSubresourceRange.baseArrayLayer = 0;
	imageSubresourceRange.layerCount = 1;

	VkImage image = crater.images[craterIndex];

	array<VkClearValue, 1> clearColors {};
	clearColors[0].color = { 0.25f, 0.25f, 0.25f, 0.0f };

	vkBeginCommandBuffer(commandBuffersArray[inFlightIndex], &bufferBeginInfo);

		VkRenderPassBeginInfo passBeginInfo { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		passBeginInfo.renderPass = lava.renderPass;
		passBeginInfo.framebuffer = framebuffers[inFlightIndex];
		passBeginInfo.renderArea.offset = { 0, 0 };
		passBeginInfo.renderArea.extent = crater.extent;
		passBeginInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());
		passBeginInfo.pClearValues = clearColors.data();

		vkCmdBeginRenderPass(commandBuffersArray[inFlightIndex], &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(commandBuffersArray[inFlightIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, lava.pipeline);

			// VkViewport viewport {};
			// viewport.x = 0.0f;
			// viewport.y = 0.0f;
			// viewport.width  = static_cast<float>(crater.extent.width);
			// viewport.height = static_cast<float>(crater.extent.height);
			// viewport.minDepth = 0.0f;
			// viewport.maxDepth = 1.0f;

			// VkRect2D scissor {};
			// scissor.offset = { 0, 0 };
			// scissor.extent = crater.extent;

			// vkCmdSetViewport(commandBuffersArray[inFlightIndex], 0, 1, &viewport); // TODO needed for dynamic pipeline parameters if they are enabled
			// vkCmdSetScissor (commandBuffersArray[inFlightIndex], 0, 1, &scissor);  // TODO needed for dynamic pipeline parameters if they are enabled

			VkBuffer vertexBuffers[] { lava.vertexBuffer };
			VkDeviceSize offsets[] { 0 };
			vkCmdBindVertexBuffers(commandBuffersArray[inFlightIndex], 0, 1, vertexBuffers, offsets);
			vkCmdBindDescriptorSets(commandBuffersArray[inFlightIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, lava.pipelineLayout, 0, 1, &descriptorSets[inFlightIndex], 0, nullptr);
			vkCmdDraw(commandBuffersArray[inFlightIndex], lava.vertexBufferSize, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffersArray[inFlightIndex]);

	vkEndCommandBuffer(commandBuffersArray[inFlightIndex]) >> ash("Failed to record command buffer!");
}

void Tectonic::drawFrame() {
	inFlightIndex = (inFlightIndex + 1) % IN_FLIGHT_FRAMES;

	uint32_t craterIndex;
	VkResult result = vkAcquireNextImageKHR(mountain.device, crater.swapChain, UINT64_MAX, imageAvailableSemaphores[inFlightIndex], VK_NULL_HANDLE, &craterIndex);
	switch (result) {
		case VK_SUCCESS:
		case VK_SUBOPTIMAL_KHR:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			cout << "TODO Window resized!" << endl;
			return;
		default:
			cout << "Problem occurred during swap chain image acquisition!" << endl;
			return;
	}

	vkWaitForFences(mountain.device, 1, &fences[inFlightIndex], VK_FALSE, UINT64_MAX) >> ash("Waiting for fence too long!");
	vkResetFences(mountain.device, 1, &fences[inFlightIndex]);

	updateUniformBuffer();
	prepareFrame(craterIndex); // TODO check for fails before continue

	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // TODO what was that: VK_PIPELINE_STAGE_TRANSFER_BIT ?
	VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphores[inFlightIndex];
	submitInfo.pWaitDstStageMask = &waitDstStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffersArray[inFlightIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderFinishedSemaphores[inFlightIndex];

	vkQueueSubmit(mountain.queue, 1, &submitInfo, fences[inFlightIndex]) >> ash("Failed to submit commands to queue!");

	VkPresentInfoKHR presentInfo { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphores[inFlightIndex];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &crater.swapChain;
	presentInfo.pImageIndices = &craterIndex;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(mountain.queue, &presentInfo);

	switch (result) {
		case VK_SUCCESS:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
		case VK_SUBOPTIMAL_KHR:
			cout << "TODO Window resized!" << endl;
			return;
		default:
			cout << "Problem occurred during image presentation!" << endl;
			return;
	}
}
