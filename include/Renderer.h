#pragma once

#include "WindowManager.h"
#include "DeviceManager.h"
#include "SwapChainManager.h"

#include <memory>
#include <vector>

namespace Vulkan3DEngine
{

	class Renderer
	{
	private:
		WindowManager& winManager;
		DeviceManager& devManager;

		std::unique_ptr<SwapChainManager> swapManager;

		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex = 0;
		int currentFrameIndex = 0;
		bool isFrameStarted = false;

	public:
		Renderer(WindowManager& winManager, DeviceManager& devManager);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		VkCommandBuffer beginFrame();
		void endFrame();
		bool isFrameInProgress() const;

		int getCurrentFrameIndex() const;

		VkCommandBuffer getCurrentCommandBuffer() const;

		void beginSwapChainRenderPass(VkCommandBuffer cmdBuffer);
		void endSwapChainRenderPass(VkCommandBuffer cmdBuffer);
		VkRenderPass getSwapChainRenderPass() const;

		float getAspectRatio() const;

	private:
		void createCommandBuffers();
		void destroyCommandBuffers();
		void recreateSwapChain();
	};

}