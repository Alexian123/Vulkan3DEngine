#pragma once

#include "DeviceManager.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace Vulkan3DEngine
{
    class DescriptorSetLayoutManager 
    {
    public:
        class Builder 
        {
        private:
            DeviceManager& devManager;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};

        public:
            Builder(DeviceManager& devManager);

            Builder& addBinding(
                uint32_t binding,
                VkDescriptorType descriptorType,
                VkShaderStageFlags stageFlags,
                uint32_t count = 1
            );
            std::unique_ptr<DescriptorSetLayoutManager> build() const;
        };

    private:
        DeviceManager& devManager;
        VkDescriptorSetLayout descriptorSetLayout;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

        friend class DescriptorWriter;

    public:
        DescriptorSetLayoutManager(
            DeviceManager& devManager, 
            std::unordered_map<uint32_t, 
            VkDescriptorSetLayoutBinding> bindings
        );
        ~DescriptorSetLayoutManager();

        DescriptorSetLayoutManager(const DescriptorSetLayoutManager&) = delete;
        DescriptorSetLayoutManager& operator=(const DescriptorSetLayoutManager&) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const;
    };

    class DescriptorPoolManager 
    {
    public:
        class Builder 
        {
        private:
            DeviceManager& devManager;
            std::vector<VkDescriptorPoolSize> poolSizes{};
            uint32_t maxSets = 1000;
            VkDescriptorPoolCreateFlags poolFlags = 0;

        public:
            Builder(DeviceManager& devManager);

            Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
            Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder& setMaxSets(uint32_t count);
            std::unique_ptr<DescriptorPoolManager> build() const;
        };

    private:
        DeviceManager& devManager;
        VkDescriptorPool descriptorPool;

        friend class DescriptorWriter;

    public:
        DescriptorPoolManager(
            DeviceManager& devManager,
            uint32_t maxSets,
            VkDescriptorPoolCreateFlags poolFlags,
            const std::vector<VkDescriptorPoolSize>& poolSizes);
        ~DescriptorPoolManager();

        DescriptorPoolManager(const DescriptorPoolManager&) = delete;
        DescriptorPoolManager& operator=(const DescriptorPoolManager&) = delete;

        bool allocateDescriptorSet(
            const VkDescriptorSetLayout descriptorSetLayout, 
            VkDescriptorSet& descriptorSet
        ) const;

        void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

        void resetPool();
    };

    class DescriptorWriter 
    {
    private:
        DescriptorSetLayoutManager& setLayout;
        DescriptorPoolManager& pool;
        std::vector<VkWriteDescriptorSet> writes;

    public:
        DescriptorWriter(DescriptorSetLayoutManager& setLayout, DescriptorPoolManager& pool);

        DescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
        DescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

        bool build(VkDescriptorSet& set);
        void overwrite(VkDescriptorSet& set);
    };

}