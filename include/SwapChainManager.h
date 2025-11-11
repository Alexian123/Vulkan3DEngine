#pragma once

#include "DeviceManager.h"

#include <string>
#include <vector>
#include <memory>

namespace Vulkan3DEngine
{

	class SwapChainManager
	{
	private:
		VkFormat swapChainImageFormat;
		VkFormat swapChainDepthFormat;
		VkExtent2D swapChainExtent;

		std::vector<VkFramebuffer> swapChainFramebuffers;
		VkRenderPass renderPass;

		std::vector<VkImage> depthImages;
		std::vector<VkDeviceMemory> depthImageMemorys;
		std::vector<VkImageView> depthImageViews;
		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;

		DeviceManager& deviceManager;
		VkExtent2D windowExtent;

		VkSwapchainKHR swapChain;

		std::shared_ptr<SwapChainManager> oldSwapChainManager;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imagesInFlight;
		size_t currentFrame = 0;

	public:
		SwapChainManager(DeviceManager& deviceManager, VkExtent2D windowExtent);
		SwapChainManager(
			DeviceManager& deviceManager, 
			VkExtent2D windowExtent, 
			std::shared_ptr<SwapChainManager> oldSwapChainManager
		);
		~SwapChainManager();

		SwapChainManager(const SwapChainManager&) = delete;
		void operator=(const SwapChainManager&) = delete;

		VkFramebuffer getFrameBuffer(int index);
		VkRenderPass getRenderPass();
		VkImageView getImageView(int index);
		size_t getImageCount();
		VkFormat getSwapChainImageFormat();
		VkExtent2D getSwapChainExtent();
		uint32_t width();
		uint32_t height();

		float extentAspectRatio();
		VkFormat findDepthFormat();

		VkResult acquireNextImage(uint32_t* imageIndex);
		VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

		bool areSwapChainFormatsEqual(const SwapChainManager& swapChainManager) const;

	private:
		void init();
		void createSwapChain();
		void createImageViews();
		void createDepthResources();
		void createRenderPass();
		void createFramebuffers();
		void createSyncObjects();

		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	};

}