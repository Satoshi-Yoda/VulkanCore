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
    // mat4 model;
    // mat4 view;
    // mat4 proj;
    vec2 scale;
    vec2 shift;
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
		if (commandBuffers[i]           != VK_NULL_HANDLE) vkFreeCommandBuffers(mountain.device, mountain.commandPool, 1, &commandBuffers[i]);
		if (imageAvailableSemaphores[i] != VK_NULL_HANDLE) vkDestroySemaphore  (mountain.device, imageAvailableSemaphores[i], nullptr);
		if (renderFinishedSemaphores[i] != VK_NULL_HANDLE) vkDestroySemaphore  (mountain.device, renderFinishedSemaphores[i], nullptr);
		if (fences[i]                   != VK_NULL_HANDLE) vkDestroyFence      (mountain.device, fences[i], nullptr);
		if (framebuffers[i]             != VK_NULL_HANDLE) vkDestroyFramebuffer(mountain.device, framebuffers[i], nullptr);
		// if (uniformBuffers[i]           != VK_NULL_HANDLE) vkDestroyBuffer     (mountain.device, uniformBuffers[i], nullptr);
		// if (uniformBuffersMemory[i]     != VK_NULL_HANDLE) vkFreeMemory        (mountain.device, uniformBuffersMemory[i], nullptr);

		vmaDestroyBuffer(mountain.allocator, uniformBuffers[i], uniformBuffersAllocations[i]);
	}
}

void Tectonic::createInFlightResources() {
	VkCommandBufferAllocateInfo allocateInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandPool = mountain.commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = IN_FLIGHT_FRAMES;

	vkAllocateCommandBuffers(mountain.device, &allocateInfo, commandBuffers.data()) >> ash("Failed to allocate command buffers!");

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
		// rocks.createBuffer(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
		rocks.createBufferVMA(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, uniformBuffers[i], uniformBuffersAllocations[i], uniformBuffersAllocationInfos[i]);
	}
}

void Tectonic::resizeDescriptorSets(size_t size) {
	if (descriptorSets.size() == size) return;

	descriptorSets.resize(size);

	for (auto& inFlightDescriptorSetsArray : descriptorSets) {
		vector<VkDescriptorSetLayout> layouts(IN_FLIGHT_FRAMES, lava.descriptorSetLayout2);
		VkDescriptorSetAllocateInfo allocInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = mountain.descriptorPool;
		allocInfo.descriptorSetCount = IN_FLIGHT_FRAMES;
		allocInfo.pSetLayouts = layouts.data();

		vkAllocateDescriptorSets(mountain.device, &allocInfo, inFlightDescriptorSetsArray.data()) >> ash("Failed to allocate descriptor set!");
	}

	// TODO slightly faster (4700 -> 4800 fps)
	// for (size_t frameIndex = 0; frameIndex < IN_FLIGHT_FRAMES; frameIndex++) {
	// 	for (size_t textureIndex = 0; textureIndex < lava.textureImageViews.size(); textureIndex++) {
	// 		updateDescriptorSet(frameIndex, textureIndex, lava.textureImageViews[textureIndex]);
	// 	}
	// }
}

void Tectonic::updateInFlightUniformBuffer() {
	// static auto startTime = chrono::high_resolution_clock::now();
	// static auto  lastTime = chrono::high_resolution_clock::now();
	// auto      currentTime = chrono::high_resolution_clock::now();
	// float time = chrono::duration<float, chrono::seconds::period>(currentTime - startTime).count();
	// float dt = chrono::duration<float, chrono::seconds::period>(currentTime - lastTime).count();
	// lastTime = currentTime;

	UniformBufferObject ubo {};
	ubo.shift = { 0.0f, 0.0f };
	ubo.scale = { 2.0f / (crater.extent.width + 1), 2.0f / (crater.extent.height + 1) };

	memcpy(uniformBuffersAllocationInfos[inFlightIndex].pMappedData, &ubo, sizeof(ubo));
}

void Tectonic::updateDescriptorSet(size_t frameIndex, size_t textureIndex, VkImageView& imageView) {
	VkDescriptorImageInfo imageInfo {};
	imageInfo.sampler = lava.textureSampler;
	imageInfo.imageView = imageView;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorBufferInfo uniformInfo {};
	uniformInfo.buffer = uniformBuffers[frameIndex];
	uniformInfo.offset = 0;
	uniformInfo.range = sizeof(UniformBufferObject);

	VkWriteDescriptorSet imageWrite { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	imageWrite.dstSet = descriptorSets[textureIndex][frameIndex];
	imageWrite.dstBinding = 0;
	imageWrite.dstArrayElement = 0;
	imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	imageWrite.descriptorCount = 1;
	imageWrite.pImageInfo = &imageInfo;

	VkWriteDescriptorSet uniformWrite { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	uniformWrite.dstSet = descriptorSets[textureIndex][frameIndex];
	uniformWrite.dstBinding = 1;
	uniformWrite.dstArrayElement = 0;
	uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformWrite.descriptorCount = 1;
	uniformWrite.pBufferInfo = &uniformInfo;

	array<VkWriteDescriptorSet, 2> descriptorWrites { imageWrite, uniformWrite };

	vkUpdateDescriptorSets(mountain.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Tectonic::prepareFrame(uint32_t craterIndex) {
	if (framebuffers[inFlightIndex] != VK_NULL_HANDLE) {
		vkDestroyFramebuffer(mountain.device, framebuffers[inFlightIndex], nullptr);
	}

	// TODO
	// I can draw to separate single (or IN_FLIGHT_FRAMES?) image with commandBuffer with a lot of operations,
	// and then copy it to dedicated swapChain image with prepared short commandBuffer

	// TODO why #framebuffers != crater.chainSize and not stored in crater? (but instead it == IN_FLIGHT_FRAMES and stored here)
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
	bufferBeginInfo.pInheritanceInfo = nullptr; // TODO used for secondary command buffers

	// VkImageSubresourceRange imageSubresourceRange {};
	// imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	// imageSubresourceRange.baseMipLevel = 0;
	// imageSubresourceRange.levelCount = 1;
	// imageSubresourceRange.baseArrayLayer = 0;
	// imageSubresourceRange.layerCount = 1;

	// VkImage image = crater.images[craterIndex];

	array<VkClearValue, 1> clearColors {};
	clearColors[0].color = { 0.25f, 0.25f, 0.25f, 0.0f };

	vkBeginCommandBuffer(commandBuffers[inFlightIndex], &bufferBeginInfo);

		// VkImageMemoryBarrier barrierFromPresentToDraw { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		// barrierFromPresentToDraw.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		// barrierFromPresentToDraw.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// barrierFromPresentToDraw.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// barrierFromPresentToDraw.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		// barrierFromPresentToDraw.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		// barrierFromPresentToDraw.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		// barrierFromPresentToDraw.image = image;
		// barrierFromPresentToDraw.subresourceRange = imageSubresourceRange;

		// vkCmdPipelineBarrier(commandBuffers[inFlightIndex], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromPresentToDraw);
		// vkCmdPipelineBarrier(commandBuffers[inFlightIndex], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromPresentToDraw);

		VkRenderPassBeginInfo passBeginInfo { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		passBeginInfo.renderPass = lava.renderPass;
		passBeginInfo.framebuffer = framebuffers[inFlightIndex];
		passBeginInfo.renderArea.offset = { 0, 0 };
		passBeginInfo.renderArea.extent = crater.extent;
		passBeginInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());
		passBeginInfo.pClearValues = clearColors.data();

		vkCmdBeginRenderPass(commandBuffers[inFlightIndex], &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// TODO can go near the vkCmdBindDescriptorSets for clarity
			vkCmdBindPipeline(commandBuffers[inFlightIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, lava.pipeline);

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

			vkCmdSetViewport(commandBuffers[inFlightIndex], 0, 1, &viewport);
			vkCmdSetScissor (commandBuffers[inFlightIndex], 0, 1, &scissor);

			resizeDescriptorSets(lava.textureImageViews.size());

			for (size_t i = 0; i < lava.textureImageViews.size(); i++) {
				VkDeviceSize offsets[] { 0 };
				vkCmdBindVertexBuffers(commandBuffers[inFlightIndex], 0, 1, &lava.vertexBuffers[i], offsets);
				vkCmdBindVertexBuffers(commandBuffers[inFlightIndex], 1, 1, &lava.instanceBuffers[i], offsets);
				updateDescriptorSet(inFlightIndex, i, lava.textureImageViews[i]);
				vkCmdBindDescriptorSets(commandBuffers[inFlightIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, lava.pipelineLayout, 0, 1, &descriptorSets[i][inFlightIndex], 0, nullptr);
				vkCmdDraw(commandBuffers[inFlightIndex], lava.vertexBufferSizes[i], lava.instanceBufferSizes[i], 0, 0);
			}

		vkCmdEndRenderPass(commandBuffers[inFlightIndex]);

		// VkImageMemoryBarrier barrierFromDrawToPresent { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		// barrierFromDrawToPresent.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// barrierFromDrawToPresent.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		// barrierFromDrawToPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		// barrierFromDrawToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		// barrierFromDrawToPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		// barrierFromDrawToPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		// barrierFromDrawToPresent.image = image;
		// barrierFromDrawToPresent.subresourceRange = imageSubresourceRange;

		// vkCmdPipelineBarrier(commandBuffers[inFlightIndex], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromDrawToPresent);
		// vkCmdPipelineBarrier(commandBuffers[inFlightIndex], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromDrawToPresent);

	vkEndCommandBuffer(commandBuffers[inFlightIndex]) >> ash("Failed to record command buffer!");
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
			crater.reinit();
			return;
		default:
			cout << "Problem occurred during swap chain image acquisition!" << endl;
			return;
	}

	vkWaitForFences(mountain.device, 1, &fences[inFlightIndex], VK_FALSE, UINT64_MAX) >> ash("Waiting for fence too long!");
	vkResetFences(mountain.device, 1, &fences[inFlightIndex]);

	updateInFlightUniformBuffer();
	prepareFrame(craterIndex); // TODO check for fails before continue

	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // TODO what was that: VK_PIPELINE_STAGE_TRANSFER_BIT ?
	VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphores[inFlightIndex];
	submitInfo.pWaitDstStageMask = &waitDstStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[inFlightIndex];
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

	// TODO check if this is needed
	// if (mountain.framebufferResized == true) { creater.reinit(); return; }

	switch (result) {
		case VK_SUCCESS:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
		case VK_SUBOPTIMAL_KHR:
			crater.reinit();
			return;
		default:
			cout << "Problem occurred during image presentation!" << endl;
			return;
	}

	// TODO
	// Sascha Willems just use that...
	// Check what was there about "extensive" sync?
	//
	// Actually there is some performance drop:
	//		60 -> 59 for a lot of static sprites
	//		7700 -> 5600 for 300 sprites
	//		60 -> 50 for a lot of stream sprites
	//
	// So, maybe use it only when windowed AND vsync.
	// vkQueueWaitIdle(mountain.queue); 
}
