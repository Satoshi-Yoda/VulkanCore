#include "Tectonic.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cassert>
#include <chrono>
#include <vector>

using namespace std;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::radians;

Tectonic::Tectonic(Ash &ash, Mountain &mountain, Rocks &rocks, Crater &crater, Lava &lava) : ash(ash), mountain(mountain), rocks(rocks), crater(crater), lava(lava) {
	createInFlightResources();
}

Tectonic::~Tectonic() {
	if (mountain.device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(mountain.device);
	}

	if (commandBuffer           != VK_NULL_HANDLE) vkFreeCommandBuffers(mountain.device, mountain.commandPool, 1, &commandBuffer);
	if (imageAvailableSemaphore != VK_NULL_HANDLE) vkDestroySemaphore  (mountain.device, imageAvailableSemaphore, nullptr);
	if (renderFinishedSemaphore != VK_NULL_HANDLE) vkDestroySemaphore  (mountain.device, renderFinishedSemaphore, nullptr);
	if (fence                   != VK_NULL_HANDLE) vkDestroyFence      (mountain.device, fence, nullptr);
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
}

void Tectonic::updateInFlightUniformBuffer() {
	UniformBufferObject ubo {};
	ubo.shift = { 0.0f, 0.0f };
	ubo.scale = { 2.0f / (crater.extent.width + 1), 2.0f / (crater.extent.height + 1) };

	memcpy(lava.uniformBuffersAllocationInfo.pMappedData, &ubo, sizeof(ubo));
}

void Tectonic::prepareFrame(uint32_t craterIndex) {
	// TODO
	// I can draw to separate single image with commandBuffer with a lot of operations,
	// and then copy it to dedicated swapChain image with prepared short commandBuffer

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
		passBeginInfo.renderPass = crater.renderPass;
		passBeginInfo.framebuffer = crater.framebuffers[craterIndex];
		passBeginInfo.renderArea.offset = { 0, 0 };
		passBeginInfo.renderArea.extent = crater.extent;
		passBeginInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());
		passBeginInfo.pClearValues = clearColors.data();

		vkCmdBeginRenderPass(commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
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

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lava.batchLayout.pipeline);
			for (auto& batch : lava.batches) {
				if (batch->instanceCount == 0) continue;

				VkDeviceSize offsets[] { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &batch->vertexBuffer, offsets);
				vkCmdBindVertexBuffers(commandBuffer, 1, 1, &batch->instanceBuffer, offsets);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lava.batchLayout.pipelineLayout, 0, 1, &batch->descriptorSet, 0, nullptr);
				vkCmdDraw(commandBuffer, batch->vertexCount, batch->instanceCount, 0, 0);
			}

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lava.rectangleLayout.pipeline);
			for (auto& rectangle : lava.rectangles) {
				VkDeviceSize offsets[] { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &rectangle->vertexBuffer, offsets);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lava.rectangleLayout.pipelineLayout, 0, 1, &rectangle->descriptorSet, 0, nullptr);
				vkCmdDraw(commandBuffer, rectangle->vertexCount, 1, 0, 0);
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
