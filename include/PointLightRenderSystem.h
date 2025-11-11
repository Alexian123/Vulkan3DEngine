#pragma once

#include "RenderSystem.h"

namespace Vulkan3DEngine
{

	class PointLightRenderSystem : public RenderSystem
	{
	public:
		static std::unique_ptr<PointLightRenderSystem> create(
			DeviceManager& devManager,
			VkRenderPass renderPass,
			VkDescriptorSetLayout globalSetLayout,
			const std::string& vertexShaderPath = "shaders/point_light_vert.spv",
			const std::string& fragmentShaderPath = "shaders/point_light_frag.spv"
		);

		~PointLightRenderSystem();

		PointLightRenderSystem(const PointLightRenderSystem&) = delete;
		PointLightRenderSystem& operator=(const PointLightRenderSystem&) = delete;

		void render(FrameData& frameData) override;
		void update(FrameData& frameData, GlobalUbo& ubo) override;

	private:
		PointLightRenderSystem(DeviceManager& devManager);

		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);

		void createPipeline(
			VkRenderPass renderPass,
			const std::string& vertexShaderPath,
			const std::string& fragmentShaderPath
		);

	};

}