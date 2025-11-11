#pragma once

#include "RenderSystem.h"

namespace Vulkan3DEngine
{

	class SimpleRenderSystem : public RenderSystem
	{
	public:
		static std::unique_ptr<SimpleRenderSystem> create(
			DeviceManager& devManager,
			VkRenderPass renderPass,
			VkDescriptorSetLayout globalSetLayout,
			const std::string& vertexShaderPath = "shaders/simple_vert.spv",
			const std::string& fragmentShaderPath = "shaders/simple_frag.spv"
		);

		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		void render(FrameData& frameData) override;
		void update(FrameData& frameData, GlobalUbo& ubo) override;

	private:
		SimpleRenderSystem(DeviceManager& devManager);

		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);

		void createPipeline(
			VkRenderPass renderPass,
			const std::string& vertexShaderPath,
			const std::string& fragmentShaderPath
		);
		
	};

}