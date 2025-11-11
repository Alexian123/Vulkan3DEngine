#include "Descriptors.h"

#include <cassert>
#include <stdexcept>

namespace Vulkan3DEngine
{

	DescriptorSetLayoutManager::Builder::Builder(DeviceManager& devManager) : devManager{ devManager } 
	{
	}

	DescriptorSetLayoutManager::Builder& DescriptorSetLayoutManager::Builder::addBinding(
		uint32_t binding, 
		VkDescriptorType descriptorType, 
		VkShaderStageFlags stageFlags, 
		uint32_t count
	)
	{
		assert(bindings.count(binding) == 0 && "Binding already in use");
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = binding;
		layoutBinding.descriptorType = descriptorType;
		layoutBinding.descriptorCount = count;
		layoutBinding.stageFlags = stageFlags;
		bindings[binding] = layoutBinding;
		return *this;
	}

	std::unique_ptr<DescriptorSetLayoutManager> DescriptorSetLayoutManager::Builder::build() const
	{
		return std::make_unique<DescriptorSetLayoutManager>(devManager, bindings);
	}

	DescriptorSetLayoutManager::DescriptorSetLayoutManager(
		DeviceManager& devManager, 
		std::unordered_map<uint32_t, 
		VkDescriptorSetLayoutBinding> bindings
	) : devManager{ devManager }, bindings{ bindings }
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
		for (auto kv : bindings) {
			setLayoutBindings.push_back(kv.second);
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
		descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

		if (vkCreateDescriptorSetLayout(
			devManager.getDeviceHandle(),
			&descriptorSetLayoutInfo,
			nullptr,
			&descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout");
		}
	}

	DescriptorSetLayoutManager::~DescriptorSetLayoutManager()
	{
		vkDestroyDescriptorSetLayout(devManager.getDeviceHandle(), descriptorSetLayout, nullptr);
	}

	VkDescriptorSetLayout DescriptorSetLayoutManager::getDescriptorSetLayout() const
	{
		return descriptorSetLayout;
	}

	DescriptorPoolManager::Builder::Builder(DeviceManager& devManager) : devManager{ devManager }
	{
	}

	DescriptorPoolManager::Builder& DescriptorPoolManager::Builder::addPoolSize(
		VkDescriptorType descriptorType, 
		uint32_t count
	)
	{
		poolSizes.push_back({ descriptorType, count });
		return *this;
	}

	DescriptorPoolManager::Builder& DescriptorPoolManager::Builder::setPoolFlags(VkDescriptorPoolCreateFlags flags)
	{
		poolFlags = flags;
		return *this;
	}

	DescriptorPoolManager::Builder& DescriptorPoolManager::Builder::setMaxSets(uint32_t count)
	{
		maxSets = count;
		return *this;
	}

	std::unique_ptr<DescriptorPoolManager> DescriptorPoolManager::Builder::build() const
	{
		return std::make_unique<DescriptorPoolManager>(devManager, maxSets, poolFlags, poolSizes);
	}

	DescriptorPoolManager::DescriptorPoolManager(
		DeviceManager& devManager, 
		uint32_t maxSets, 
		VkDescriptorPoolCreateFlags poolFlags, 
		const std::vector<VkDescriptorPoolSize>& poolSizes
	) : devManager{ devManager }
	{
		VkDescriptorPoolCreateInfo descriptorPoolInfo{};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		descriptorPoolInfo.pPoolSizes = poolSizes.data();
		descriptorPoolInfo.maxSets = maxSets;
		descriptorPoolInfo.flags = poolFlags;

		if (vkCreateDescriptorPool(devManager.getDeviceHandle(), &descriptorPoolInfo, nullptr, &descriptorPool) !=
			VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor pool");
		}
	}

	DescriptorPoolManager::~DescriptorPoolManager()
	{
		vkDestroyDescriptorPool(devManager.getDeviceHandle(), descriptorPool, nullptr);
	}

	bool DescriptorPoolManager::allocateDescriptorSet(
		const VkDescriptorSetLayout descriptorSetLayout, 
		VkDescriptorSet& descriptorSet
	) const
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.pSetLayouts = &descriptorSetLayout;
		allocInfo.descriptorSetCount = 1;

		// Might want to create a class that handles this case, and builds a new pool whenever an old pool fills up
		if (vkAllocateDescriptorSets(devManager.getDeviceHandle(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
			return false;
		}
		return true;
	}

	void DescriptorPoolManager::freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const
	{
		vkFreeDescriptorSets(
			devManager.getDeviceHandle(),
			descriptorPool,
			static_cast<uint32_t>(descriptors.size()),
			descriptors.data()
		);
	}

	void DescriptorPoolManager::resetPool()
	{
		vkResetDescriptorPool(devManager.getDeviceHandle(), descriptorPool, 0);
	}


	DescriptorWriter::DescriptorWriter(DescriptorSetLayoutManager& setLayout, DescriptorPoolManager& pool)
		: setLayout{ setLayout }, pool{ pool }
	{
	}

	DescriptorWriter& DescriptorWriter::writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo)
	{
		assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

		auto& bindingDescription = setLayout.bindings[binding];

		assert(
			bindingDescription.descriptorCount == 1 &&
			"Binding single descriptor info, but binding expects multiple"
		);

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding;
		write.pBufferInfo = bufferInfo;
		write.descriptorCount = 1;

		writes.push_back(write);
		return *this;
	}

	DescriptorWriter& DescriptorWriter::writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo)
	{
		assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

		auto& bindingDescription = setLayout.bindings[binding];

		assert(
			bindingDescription.descriptorCount == 1 &&
			"Binding single descriptor info, but binding expects multiple"
		);

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding;
		write.pImageInfo = imageInfo;
		write.descriptorCount = 1;

		writes.push_back(write);
		return *this;
	}

	bool DescriptorWriter::build(VkDescriptorSet& set)
	{
		bool success = pool.allocateDescriptorSet(setLayout.getDescriptorSetLayout(), set);
		if (!success) {
			return false;
		}
		overwrite(set);
		return true;
	}

	void DescriptorWriter::overwrite(VkDescriptorSet& set)
	{
		for (auto& write : writes) {
			write.dstSet = set;
		}
		vkUpdateDescriptorSets(pool.devManager.getDeviceHandle(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}
}