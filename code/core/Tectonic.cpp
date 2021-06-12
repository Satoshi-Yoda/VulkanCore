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

	if (commandBuffer           != VK_NULL_HANDLE) vkFreeCommandBuffers(mountain.device, mountain.commandPool, 1, &commandBuffer);
	if (imageAvailableSemaphore != VK_NULL_HANDLE) vkDestroySemaphore  (mountain.device, imageAvailableSemaphore, nullptr);
	if (renderFinishedSemaphore != VK_NULL_HANDLE) vkDestroySemaphore  (mountain.device, renderFinishedSemaphore, nullptr);
	if (fence                   != VK_NULL_HANDLE) vkDestroyFence      (mountain.device, fence, nullptr);
	if (framebuffer             != VK_NULL_HANDLE) vkDestroyFramebuffer(mountain.device, framebuffer, nullptr);

	vmaDestroyBuffer(mountain.allocator, uniformBuffer, uniformBuffersAllocation);
}

void Tectonic::createInFlightResources() {
	VkCommandBufferAllocateInfo allocateInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandPool = mountain.commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(mountain.device, &allocateInfo, &commandBuffer) >> ash("Failed to allocate command buffers!");

	VkSemaphoreCreateInfo semaphore_create_info { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	vkCreateSemaphore(mountain.device, &semaphore_create_info, nullptr, &imageAvailableSemaphore) >> ash("Failed to create semaphore!");
	vkCreateSemaphore(mountain.device, &semaphore_create_info, nullptr, &renderFinishedSemaphore) >> ash("Failed to create semaphore!");

	VkFenceCreateInfo fenceInfo { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	vkCreateFence(mountain.device, &fenceInfo, nullptr, &fence) >> ash("Failed to create fence!");

	framebuffer = VK_NULL_HANDLE;
}

void Tectonic::createUniformBuffers() {
	rocks.createBufferVMA(sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_ONLY, uniformBuffer, uniformBuffersAllocation, uniformBuffersAllocationInfo);
}

void Tectonic::resizeDescriptorSets(size_t size) {
	if (descriptorSets.size() == size) return;

	descriptorSets.resize(size);

	for (auto& descriptorSet : descriptorSets) {
		VkDescriptorSetLayout layout { lava.descriptorSetLayout };
		VkDescriptorSetAllocateInfo allocInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = mountain.descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;

		vkAllocateDescriptorSets(mountain.device, &allocInfo, &descriptorSet) >> ash("Failed to allocate descriptor set!");
	}

	// TODO slightly faster (4700 -> 4800 fps)
	// for (size_t frameIndex = 0; frameIndex < 1; frameIndex++) {
	// 	for (size_t textureIndex = 0; textureIndex < lava.textureImageViews.size(); textureIndex++) {
	// 		updateDescriptorSet(frameIndex, textureIndex, lava.textureImageViews[textureIndex]);
	// 	}
	// }
}

void Tectonic::updateInFlightUniformBuffer() {
	UniformBufferObject ubo {};
	ubo.shift = { 0.0f, 0.0f };
	ubo.scale = { 2.0f / (crater.extent.width + 1), 2.0f / (crater.extent.height + 1) };

	memcpy(uniformBuffersAllocationInfo.pMappedData, &ubo, sizeof(ubo));
}

// TODO maybe this won't be needed when there will be only one in-flight frame
void Tectonic::updateDescriptorSet(size_t textureIndex, VkImageView& imageView) {
	VkDescriptorImageInfo imageInfo {};
	imageInfo.sampler = lava.textureSampler;
	imageInfo.imageView = imageView;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorBufferInfo uniformInfo {};
	uniformInfo.buffer = uniformBuffer;
	uniformInfo.offset = 0;
	uniformInfo.range = sizeof(UniformBufferObject);

	VkWriteDescriptorSet imageWrite { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	imageWrite.dstSet = descriptorSets[textureIndex];
	imageWrite.dstBinding = 0;
	imageWrite.dstArrayElement = 0;
	imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	imageWrite.descriptorCount = 1;
	imageWrite.pImageInfo = &imageInfo;

	VkWriteDescriptorSet uniformWrite { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	uniformWrite.dstSet = descriptorSets[textureIndex];
	uniformWrite.dstBinding = 1;
	uniformWrite.dstArrayElement = 0;
	uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformWrite.descriptorCount = 1;
	uniformWrite.pBufferInfo = &uniformInfo;

	array<VkWriteDescriptorSet, 2> descriptorWrites { imageWrite, uniformWrite };

	vkUpdateDescriptorSets(mountain.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Tectonic::prepareFrame(uint32_t craterIndex) {
	if (framebuffer != VK_NULL_HANDLE) {
		vkDestroyFramebuffer(mountain.device, framebuffer, nullptr);
	}

	// TODO
	// I can draw to separate single image with commandBuffer with a lot of operations,
	// and then copy it to dedicated swapChain image with prepared short commandBuffer

	// TODO why #framebuffers != crater.chainSize and not stored in crater? (but instead it == # in flight frames and stored here)
	VkFramebufferCreateInfo framebufferCreateInfo { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferCreateInfo.renderPass = lava.renderPass;
	framebufferCreateInfo.attachmentCount = 1;
	framebufferCreateInfo.pAttachments = &crater.imageViews[craterIndex];
	framebufferCreateInfo.width  = crater.extent.width;
	framebufferCreateInfo.height = crater.extent.height;
	framebufferCreateInfo.layers = 1;

	vkCreateFramebuffer(mountain.device, &framebufferCreateInfo, nullptr, &framebuffer) >> ash("Failed to create framebuffer!");

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

	vkBeginCommandBuffer(commandBuffer, &bufferBeginInfo);

		// VkImageMemoryBarrier barrierFromPresentToDraw { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		// barrierFromPresentToDraw.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		// barrierFromPresentToDraw.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// barrierFromPresentToDraw.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// barrierFromPresentToDraw.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		// barrierFromPresentToDraw.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		// barrierFromPresentToDraw.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		// barrierFromPresentToDraw.image = image;
		// barrierFromPresentToDraw.subresourceRange = imageSubresourceRange;

		// vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromPresentToDraw);
		// vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromPresentToDraw);

		VkRenderPassBeginInfo passBeginInfo { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		passBeginInfo.renderPass = lava.renderPass;
		passBeginInfo.framebuffer = framebuffer;
		passBeginInfo.renderArea.offset = { 0, 0 };
		passBeginInfo.renderArea.extent = crater.extent;
		passBeginInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());
		passBeginInfo.pClearValues = clearColors.data();

		vkCmdBeginRenderPass(commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// TODO can go near the vkCmdBindDescriptorSets for clarity
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lava.pipeline);

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

			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			vkCmdSetScissor (commandBuffer, 0, 1, &scissor);

			resizeDescriptorSets(lava.caves.size());

			for (size_t i = 0; i < lava.caves.size(); i++) {
				if (lava.caves[i]->instanceCount == 0) continue;

				VkDeviceSize offsets[] { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &lava.caves[i]->vertexBuffer, offsets);
				vkCmdBindVertexBuffers(commandBuffer, 1, 1, &lava.caves[i]->instanceBuffer, offsets);
				updateDescriptorSet(i, lava.caves[i]->textureView);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lava.pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
				vkCmdDraw(commandBuffer, lava.caves[i]->vertexCount, lava.caves[i]->instanceCount, 0, 0);
			}

		vkCmdEndRenderPass(commandBuffer);

		// VkImageMemoryBarrier barrierFromDrawToPresent { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		// barrierFromDrawToPresent.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		// barrierFromDrawToPresent.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		// barrierFromDrawToPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		// barrierFromDrawToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		// barrierFromDrawToPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		// barrierFromDrawToPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		// barrierFromDrawToPresent.image = image;
		// barrierFromDrawToPresent.subresourceRange = imageSubresourceRange;

		// vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromDrawToPresent);
		// vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierFromDrawToPresent);

	vkEndCommandBuffer(commandBuffer) >> ash("Failed to record command buffer!");
}

void Tectonic::drawFrame() {
	uint32_t craterIndex;
	VkResult result = vkAcquireNextImageKHR(mountain.device, crater.swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &craterIndex);
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

	vkWaitForFences(mountain.device, 1, &fence, VK_FALSE, UINT64_MAX) >> ash("Waiting for fence too long!");
	vkResetFences(mountain.device, 1, &fence);

	updateInFlightUniformBuffer();
	prepareFrame(craterIndex); // TODO check for fails before continue

	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // TODO what was that: VK_PIPELINE_STAGE_TRANSFER_BIT ?
	VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
	submitInfo.pWaitDstStageMask = &waitDstStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

	vkQueueSubmit(mountain.queue, 1, &submitInfo, fence) >> ash("Failed to submit commands to queue!");

	VkPresentInfoKHR presentInfo { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
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
