#include "RenderSystem.h"

namespace Vulkan3DEngine
{

	RenderSystem::RenderSystem(DeviceManager& devManager) : devManager{ devManager }
	{
	}

	void RenderSystem::init(
		VkRenderPass renderPass, 
		VkDescriptorSetLayout globalSetLayout, 
		const std::string& vertexShaderPath, 
		const std::string& fragmentShaderPath
	)
	{
		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass, vertexShaderPath, fragmentShaderPath);
	}


	RenderSystem::~RenderSystem()
	{
		if (pipelineLayout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(devManager.getDeviceHandle(), pipelineLayout, nullptr);
		}
	}
}