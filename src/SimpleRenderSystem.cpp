#include "SimpleRenderSystem.h"
#include "EntityComponents.h"
#include "MathUtils.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>

namespace Vulkan3DEngine
{
	struct SimplePushConstantData
	{
		glm::mat4 modelMatrix{ 1.f };
		glm::mat4 normalMatrix{ 1.f };
	};

	std::unique_ptr<SimpleRenderSystem> SimpleRenderSystem::create(
		DeviceManager& devManager, 
		VkRenderPass renderPass, 
		VkDescriptorSetLayout globalSetLayout, 
		const std::string& vertexShaderPath, 
		const std::string& fragmentShaderPath
	)
	{
		auto simpleRenderSystem = std::unique_ptr<SimpleRenderSystem>(new SimpleRenderSystem(devManager));
		simpleRenderSystem->init(renderPass, globalSetLayout, vertexShaderPath, fragmentShaderPath);
		return simpleRenderSystem;
	}

	SimpleRenderSystem::SimpleRenderSystem(DeviceManager& devManager) : RenderSystem(devManager)
	{
	}

	SimpleRenderSystem::~SimpleRenderSystem()
	{
	}

	void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

		VkPipelineLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		createInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		createInfo.pSetLayouts = descriptorSetLayouts.data();
		createInfo.pushConstantRangeCount = 1;
		createInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(devManager.getDeviceHandle(), &createInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout");
		}
	}

	void SimpleRenderSystem::createPipeline(
		VkRenderPass renderPass,
		const std::string& vertexShaderPath,
		const std::string& fragmentShaderPath
	)
	{
		assert(pipelineLayout != nullptr && "Pipeline cannot be created before the pipeline layout");
		PipelineConfigInfo pipelineConfig{};
		GfxPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		gfxPipeline = std::make_unique<GfxPipeline>(
			devManager,
			vertexShaderPath,
			fragmentShaderPath,
			pipelineConfig
		);
	}

	void SimpleRenderSystem::render(FrameData& frameData)
	{
		gfxPipeline->bind(frameData.cmdBuffer);

		vkCmdBindDescriptorSets(
			frameData.cmdBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&frameData.globalDescSet,
			0,
			nullptr
		);

		for (auto& kvPair : frameData.entities) {
			auto& obj = kvPair.second;
			if (!obj.hasComponent<ModelComponent>() || !obj.hasComponent<TransformComponent>()) continue;

			const TransformComponent& transform = *obj.getComponent<TransformComponent>();
			SimplePushConstantData pushConstant{};
			pushConstant.modelMatrix = MathUtils::createTransformationMatrix(
				transform.translation,
				transform.rotation,
				transform.scale
			);
			pushConstant.normalMatrix = MathUtils::createNormalMatrix(
				transform.rotation,
				transform.scale
			);

			vkCmdPushConstants(
				frameData.cmdBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&pushConstant
			);

			const ModelComponent& modelComp = *obj.getComponent<ModelComponent>();
			modelComp.model->bind(frameData.cmdBuffer);
			modelComp.model->draw(frameData.cmdBuffer);
		}
	}

	void SimpleRenderSystem::update(FrameData& frameData, GlobalUbo& ubo)
	{
	}
}