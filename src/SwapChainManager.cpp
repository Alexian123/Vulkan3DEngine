#include "SwapChainManager.h"

#include "Constants.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace Vulkan3DEngine
{
	SwapChainManager::SwapChainManager(DeviceManager& deviceManager, VkExtent2D windowExtent)
		: deviceManager{ deviceManager }, windowExtent{ windowExtent }
	{
		init();
	}

	SwapChainManager::SwapChainManager(
		DeviceManager& deviceManager, 
		VkExtent2D windowExtent, 
		std::shared_ptr<SwapChainManager> oldSwapChainManager
	) : deviceManager{ deviceManager }, windowExtent{ windowExtent }, oldSwapChainManager{ oldSwapChainManager }
	{
		init();
		oldSwapChainManager = nullptr;
	}

	SwapChainManager::~SwapChainManager()
	{
		VkDevice device = deviceManager.getDeviceHandle();

		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}
		swapChainImageViews.clear();

		if (swapChain != nullptr) {
			vkDestroySwapchainKHR(device, swapChain, nullptr);
			swapChain = nullptr;
		}

		for (int i = 0; i < depthImages.size(); i++) {
			vkDestroyImageView(device, depthImageViews[i], nullptr);
			vkDestroyImage(device, depthImages[i], nullptr);
			vkFreeMemory(device, depthImageMemorys[i], nullptr);
		}

		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		vkDestroyRenderPass(device, renderPass, nullptr);

		// cleanup synchronization objects
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		}
		for (size_t i = 0; i < AppConstants::MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}
	}

	VkFramebuffer SwapChainManager::getFrameBuffer(int index)
	{
		return swapChainFramebuffers[index];
	}

	VkRenderPass SwapChainManager::getRenderPass()
	{
		return renderPass;
	}

	VkImageView SwapChainManager::getImageView(int index)
	{
		return swapChainImageViews[index];
	}

	size_t SwapChainManager::getImageCount()
	{
		return swapChainImages.size();
	}

	VkFormat SwapChainManager::getSwapChainImageFormat()
	{
		return swapChainImageFormat;
	}

	VkExtent2D SwapChainManager::getSwapChainExtent()
	{
		return swapChainExtent;
	}

	uint32_t SwapChainManager::width()
	{
		return swapChainExtent.width;
	}

	uint32_t SwapChainManager::height()
	{
		return swapChainExtent.height;
	}

	float SwapChainManager::extentAspectRatio()
	{
		return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
	}

	VkFormat SwapChainManager::findDepthFormat()
	{
		return deviceManager.findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	VkResult SwapChainManager::acquireNextImage(uint32_t* imageIndex)
	{
		vkWaitForFences(
			deviceManager.getDeviceHandle(),
			1,
			&inFlightFences[currentFrame],
			VK_TRUE,
			std::numeric_limits<uint64_t>::max()
		);

		VkResult result = vkAcquireNextImageKHR(
			deviceManager.getDeviceHandle(),
			swapChain,
			std::numeric_limits<uint64_t>::max(),
			imageAvailableSemaphores[currentFrame],  // must be a non signaled semaphore
			VK_NULL_HANDLE,
			imageIndex
		);

		return result;
	}

	VkResult SwapChainManager::submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex)
	{
		if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(deviceManager.getDeviceHandle(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
		}
		imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = buffers;

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[*imageIndex] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(deviceManager.getDeviceHandle(), 1, &inFlightFences[currentFrame]);
		if (vkQueueSubmit(
			deviceManager.getGraphicsQueueHandle(), 
			1, 
			&submitInfo, 
			inFlightFences[currentFrame]
		) != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit draw command buffer");
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = imageIndex;

		auto result = vkQueuePresentKHR(deviceManager.getPresentQueueHandle(), &presentInfo);

		currentFrame = (currentFrame + 1) % AppConstants::MAX_FRAMES_IN_FLIGHT;

		return result;
	}

	bool SwapChainManager::areSwapChainFormatsEqual(const SwapChainManager& swapChainManager) const
	{
		return swapChainManager.swapChainDepthFormat == this->swapChainDepthFormat &&
			swapChainManager.swapChainImageFormat == this->swapChainImageFormat;
	}

	void SwapChainManager::init()
	{
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDepthResources();
		createFramebuffers();
		createSyncObjects();
	}

	void SwapChainManager::createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = deviceManager.getSwapChainSupport();

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 &&
			imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = deviceManager.getSurfaceHandle();

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = deviceManager.getQueueFamilies();
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;      // Optional
			createInfo.pQueueFamilyIndices = nullptr;  // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = oldSwapChainManager ? oldSwapChainManager->swapChain : nullptr;

		if (vkCreateSwapchainKHR(deviceManager.getDeviceHandle(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create swap chain");
		}

		vkGetSwapchainImagesKHR(deviceManager.getDeviceHandle(), swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(deviceManager.getDeviceHandle(), swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void SwapChainManager::createImageViews()
	{
		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = swapChainImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = swapChainImageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(deviceManager.getDeviceHandle(), &viewInfo, nullptr, &swapChainImageViews[i]) 
				!= VK_SUCCESS) {
				throw std::runtime_error("Failed to create image view");
			}
		}
	}

	void SwapChainManager::createDepthResources()
	{
		VkFormat depthFormat = findDepthFormat();
		swapChainDepthFormat = depthFormat;
		VkExtent2D extent = getSwapChainExtent();

		depthImages.resize(getImageCount());
		depthImageMemorys.resize(getImageCount());
		depthImageViews.resize(getImageCount());

		for (int i = 0; i < depthImages.size(); i++) {
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = extent.width;
			imageInfo.extent.height = extent.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = depthFormat;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.flags = 0;

			deviceManager.createImageWithInfo(
				imageInfo,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				depthImages[i],
				depthImageMemorys[i]
			);

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = depthImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = depthFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(deviceManager.getDeviceHandle(), &viewInfo, nullptr, &depthImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create image view!");
			}
		}
	}

	void SwapChainManager::createRenderPass()
	{
		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = getSwapChainImageFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | 
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstSubpass = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | 
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | 
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(deviceManager.getDeviceHandle(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create render pass");
		}
	}

	void SwapChainManager::createFramebuffers()
	{
		swapChainFramebuffers.resize(getImageCount());
		for (size_t i = 0; i < getImageCount(); i++) {
			std::array<VkImageView, 2> attachments = { swapChainImageViews[i], depthImageViews[i] };

			VkExtent2D extent = getSwapChainExtent();
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = extent.width;
			framebufferInfo.height = extent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(deviceManager.getDeviceHandle(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) 
				!= VK_SUCCESS) {
				throw std::runtime_error("Failed to create framebuffer");
			}
		}
	}

	void SwapChainManager::createSyncObjects()
	{
		imageAvailableSemaphores.resize(AppConstants::MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(swapChainImages.size());
		inFlightFences.resize(AppConstants::MAX_FRAMES_IN_FLIGHT);
		imagesInFlight.resize(getImageCount(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < AppConstants::MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(deviceManager.getDeviceHandle(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) !=
				VK_SUCCESS ||
				vkCreateFence(deviceManager.getDeviceHandle(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create synchronization objects");
			}
		}
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			if (vkCreateSemaphore(deviceManager.getDeviceHandle(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) !=
				VK_SUCCESS) {
				throw std::runtime_error("Failed to create synchronization objects");
			}
		}
	}

	VkSurfaceFormatKHR SwapChainManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
				availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}
		return availableFormats[0];
	}

	VkPresentModeKHR SwapChainManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		// Low latency, high power consumption
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				std::cout << "Present mode: Mailbox" << std::endl;
				return availablePresentMode;
			}
		}

		// Low latency, no V-Sync, possible tearing
		// for (const auto &availablePresentMode : availablePresentModes) {
		//   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
		//     std::cout << "Present mode: Immediate" << std::endl;
		//     return availablePresentMode;
		//   }
		// }

		// Higher latency, V-Sync
		std::cout << "Present mode: V-Sync" << std::endl;
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D SwapChainManager::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		VkExtent2D actualExtent = windowExtent;
		actualExtent.width = std::max(
			capabilities.minImageExtent.width,
			std::min(capabilities.maxImageExtent.width, actualExtent.width)
		);
		actualExtent.height = std::max(
			capabilities.minImageExtent.height,
			std::min(capabilities.maxImageExtent.height, actualExtent.height)
		);
		return actualExtent;
	}
}