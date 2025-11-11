#pragma once

#include "WindowManager.h"

#include <string>
#include <vector>
#include <optional>

namespace Vulkan3DEngine
{

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
		bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
	};

	class DeviceManager
	{
	public:
#ifdef NDEBUG
		const bool enableValidationLayers = false;
#else
		const bool enableValidationLayers = true;
#endif
		VkPhysicalDeviceProperties physicalDeviceProperties;

	private:
		const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
		const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VkInstance instance;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		WindowManager& windowManager;
		VkCommandPool commandPool;

		VkDevice device;
		VkSurfaceKHR surface;
		VkQueue graphicsQueue;
		VkQueue presentQueue;

	public:
		DeviceManager(WindowManager& windowManager);
		~DeviceManager();

		DeviceManager(const DeviceManager&) = delete;
		DeviceManager& operator=(const DeviceManager&) = delete;
		DeviceManager(DeviceManager&&) = delete;
		DeviceManager& operator=(DeviceManager&&) = delete;

		VkCommandPool getCommandPoolHandle() const;
		VkDevice getDeviceHandle() const;
		VkSurfaceKHR getSurfaceHandle() const;
		VkQueue getGraphicsQueueHandle() const;
		VkQueue getPresentQueueHandle() const;

		SwapChainSupportDetails getSwapChainSupport();
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
		QueueFamilyIndices getQueueFamilies();
		VkFormat findSupportedFormat(
			const std::vector<VkFormat>& candidates, 
			VkImageTiling tiling, 
			VkFormatFeatureFlags features
		);

		void createBuffer(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkBuffer& buffer,
			VkDeviceMemory& bufferMemory
		);
		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);
		void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

		void createImageWithInfo(
			const VkImageCreateInfo& createInfo,
			VkMemoryPropertyFlags properties,
			VkImage& image,
			VkDeviceMemory& imageMemory
		);

	private:
		void createInstance();
		void setupDebugMessenger();
		void createSurface();
		void selectPhysicalDevice();
		void createLogicalDevice();
		void createCommandPool();

		bool isDeviceSuitable(VkPhysicalDevice device);
		std::vector<const char*> getRequiredExtensions();
		bool checkValidationLayerSupport();
		QueueFamilyIndices queryQueueFamilies(VkPhysicalDevice device);
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void hasGflwRequiredInstanceExtensions();
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	};

}