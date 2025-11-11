#pragma once

#include "DeviceManager.h"

namespace Vulkan3DEngine
{

	class BufferManager
	{
    private:
        DeviceManager& deviceManager;
        void* mapped = nullptr;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;

        VkDeviceSize bufferSize;
        uint32_t instanceCount;
        VkDeviceSize instanceSize;
        VkDeviceSize alignmentSize;
        VkBufferUsageFlags usageFlags;
        VkMemoryPropertyFlags memoryPropertyFlags;

	public:
        BufferManager(
            DeviceManager& deviceManager,
            VkDeviceSize instanceSize,
            uint32_t instanceCount,
            VkBufferUsageFlags usageFlags,
            VkMemoryPropertyFlags memoryPropertyFlags,
            VkDeviceSize minOffsetAlignment = 1
        );
        ~BufferManager();

        BufferManager(const BufferManager&) = delete;
        BufferManager& operator=(const BufferManager&) = delete;

        VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void unmap();

        void writeToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkDescriptorBufferInfo descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        void writeToIndex(void* data, int index);
        VkResult flushIndex(int index);
        VkDescriptorBufferInfo descriptorInfoForIndex(int index);
        VkResult invalidateIndex(int index);

        VkBuffer getBuffer() const;
        void* getMappedMemory() const;
        uint32_t getInstanceCount() const;
        VkDeviceSize getInstanceSize() const;
        VkDeviceSize getAlignmentSize() const;
        VkBufferUsageFlags getUsageFlags() const;
        VkMemoryPropertyFlags getMemoryPropertyFlags() const;
        VkDeviceSize getBufferSize() const;

    private:
        static VkDeviceSize getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment);
	};

}