#include "Renderer.h"

#include "Constants.h"

#include <stdexcept>
#include <cassert>
#include <array>

namespace Vulkan3DEngine
{
	Renderer::Renderer(WindowManager& winManager, DeviceManager& devManager)
		: winManager{ winManager }, devManager{ devManager }
	{
		recreateSwapChain();
		createCommandBuffers();
	}

	Renderer::~Renderer()
	{
		destroyCommandBuffers();
	}

	VkCommandBuffer Renderer::beginFrame()
	{
		assert(!isFrameStarted && "Cannot begin a new frame if one is already in progress");

		auto result = swapManager->acquireNextImage(&currentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return nullptr;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to acquire next image from the swap chain");
		}
		
		isFrameStarted = true;

		auto cmdBuffer = getCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(cmdBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer");
		}

		return cmdBuffer;
	}

	void Renderer::endFrame()
	{
		assert(isFrameStarted && "Cannot end a frame if it is not in progress");

		auto cmdBuffer = getCurrentCommandBuffer();
		if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer");
		}

		auto result = swapManager->submitCommandBuffers(&cmdBuffer, &currentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || winManager.windowWasResized()) {
			winManager.resetWindowResizedFlag();
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to present image from the swap chain");
		}

		isFrameStarted = false;
		currentFrameIndex = (currentFrameIndex + 1) % AppConstants::MAX_FRAMES_IN_FLIGHT;
	}

	bool Renderer::isFrameInProgress() const
	{
		return isFrameStarted;
	}

	int Renderer::getCurrentFrameIndex() const
	{
		assert(isFrameStarted && "Cannot get frame index if frame is not in progress");
		return currentFrameIndex;
	}

	VkCommandBuffer Renderer::getCurrentCommandBuffer() const
	{
		assert(isFrameStarted && "Cannot get command buffer if frame is not in progress");
		return commandBuffers[currentFrameIndex];
	}

	void Renderer::beginSwapChainRenderPass(VkCommandBuffer cmdBuffer)
	{
		assert(isFrameStarted && "Cannot begin render pass if a frame is not in progress");
		assert(
			cmdBuffer == getCurrentCommandBuffer() && 
			"Cannot begin render pass on a command buffer from a different frame"
		);

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = swapManager->getRenderPass();
		renderPassBeginInfo.framebuffer = swapManager->getFrameBuffer(currentImageIndex);
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = swapManager->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapManager->getSwapChainExtent().width);
		viewport.height = static_cast<float>(swapManager->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ { 0, 0 }, swapManager->getSwapChainExtent() };
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
	}

	void Renderer::endSwapChainRenderPass(VkCommandBuffer cmdBuffer)
	{
		assert(isFrameStarted && "Cannot end render pass if a frame is not in progress");
		assert(
			cmdBuffer == getCurrentCommandBuffer() &&
			"Cannot end render pass on a command buffer from a different frame"
		);
		vkCmdEndRenderPass(cmdBuffer);
	}

	VkRenderPass Renderer::getSwapChainRenderPass() const
	{
		return swapManager->getRenderPass();
	}

	float Renderer::getAspectRatio() const
	{
		return swapManager->extentAspectRatio();
	}

	void Renderer::createCommandBuffers()
	{
		commandBuffers.resize(AppConstants::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = devManager.getCommandPoolHandle();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(devManager.getDeviceHandle(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate command buffers");
		}
	}

	void Renderer::destroyCommandBuffers()
	{
		vkFreeCommandBuffers(
			devManager.getDeviceHandle(),
			devManager.getCommandPoolHandle(),
			static_cast<uint32_t>(commandBuffers.size()),
			commandBuffers.data()
		);
		commandBuffers.clear();
	}

	void Renderer::recreateSwapChain()
	{
		VkExtent2D extent{};
		do {
			extent = winManager.getExtent();
			glfwWaitEvents();
		} while (extent.width == 0 || extent.height == 0);

		vkDeviceWaitIdle(devManager.getDeviceHandle());

		if (swapManager == nullptr) {
			swapManager = std::make_unique<SwapChainManager>(devManager, extent);
		}
		else {
			std::shared_ptr<SwapChainManager> oldSwapManager = std::move(swapManager);
			swapManager = std::make_unique<SwapChainManager>(devManager, extent, oldSwapManager);

			if (!oldSwapManager->areSwapChainFormatsEqual(*swapManager.get())) {
				throw std::runtime_error("Swap chain image/depth format has changed");
			}
		}
	}
}