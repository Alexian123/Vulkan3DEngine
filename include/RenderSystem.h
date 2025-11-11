#pragma once

#include "GfxPipeline.h"
#include "DeviceManager.h"
#include "FrameInfo.h"

#include <string>
#include <memory>

namespace Vulkan3DEngine
{
	class RenderSystem
	{
	protected:
		DeviceManager& devManager;
		std::unique_ptr<GfxPipeline> gfxPipeline;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

	public:
		~RenderSystem();

		RenderSystem(const RenderSystem&) = delete;
		RenderSystem& operator=(const RenderSystem&) = delete;

		virtual void render(FrameData& frameData) = 0;
		virtual void update(FrameData& frameData, GlobalUbo& ubo) = 0;

	protected:
		RenderSystem(DeviceManager& devManager);

		void init(
			VkRenderPass renderPass, 
			VkDescriptorSetLayout globalSetLayout,
			const std::string& vertexShaderPath,
			const std::string& fragmentShaderPath
		);

		virtual void createPipelineLayout(VkDescriptorSetLayout globalSetLayout) = 0;

		virtual void createPipeline(
			VkRenderPass renderPass, 
			const std::string& vertexShaderPath, 
			const std::string& fragmentShaderPath
		) = 0;
	};

}