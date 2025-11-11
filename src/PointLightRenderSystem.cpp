#include "PointLightRenderSystem.h"
#include "EntityComponents.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <array>
#include <cassert>
#include <map>

namespace Vulkan3DEngine
{
	struct PointLightPushConstants
	{
		glm::vec4 position{};
		glm::vec4 color{};
		float radius;
	};

	std::unique_ptr<PointLightRenderSystem> PointLightRenderSystem::create(
		DeviceManager& devManager,
		VkRenderPass renderPass,
		VkDescriptorSetLayout globalSetLayout,
		const std::string& vertexShaderPath,
		const std::string& fragmentShaderPath
	)
	{
		auto pointLightRenderSys = std::unique_ptr<PointLightRenderSystem>(new PointLightRenderSystem(devManager));
		pointLightRenderSys->init(renderPass, globalSetLayout, vertexShaderPath, fragmentShaderPath);
		return pointLightRenderSys;
	}

	PointLightRenderSystem::PointLightRenderSystem(DeviceManager& devManager) : RenderSystem(devManager)
	{
	}

	PointLightRenderSystem::~PointLightRenderSystem()
	{
	}

	void PointLightRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PointLightPushConstants);

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

	void PointLightRenderSystem::createPipeline(
		VkRenderPass renderPass,
		const std::string& vertexShaderPath,
		const std::string& fragmentShaderPath
	)
	{
		assert(pipelineLayout != nullptr && "Pipeline cannot be created before the pipeline layout");
		PipelineConfigInfo pipelineConfig{};
		GfxPipeline::defaultPipelineConfigInfo(pipelineConfig);
		GfxPipeline::enableAlphaBlending(pipelineConfig);
		pipelineConfig.bindingDescriptions.clear();
		pipelineConfig.attribDescriptions.clear();
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		gfxPipeline = std::make_unique<GfxPipeline>(
			devManager,
			vertexShaderPath,
			fragmentShaderPath,
			pipelineConfig
		);
	}

	void PointLightRenderSystem::update(FrameData& frameData, GlobalUbo& ubo)
	{
		auto rotateLight = glm::rotate(
			glm::mat4(1.0f),
			frameData.frameTime,
			{ 0.f, -1.f, 0.f }
		);
		int lightIndex = 0;
		for (auto& kvPair : frameData.entities) {
			auto& obj = kvPair.second;
			if (!obj.hasComponent<PointLightComponent>() || !obj.hasComponent<TransformComponent>()) continue;

			assert(lightIndex < AppConstants::MAX_LIGHTS && "Exceeded max number of lights");

			// update light position
			//obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.0f));

			const TransformComponent& transform = *obj.getComponent<TransformComponent>();
			const PointLightComponent& light = *obj.getComponent<PointLightComponent>();

			// copy light to ubo
			ubo.pointLights[lightIndex].position = glm::vec4(transform.translation, 1.0f);
			ubo.pointLights[lightIndex].color = glm::vec4(light.color, light.intensity);
			++lightIndex;
		}
		ubo.numLights = lightIndex;
	}

	void PointLightRenderSystem::render(FrameData& frameData)
	{
		// sort lights based on distance to camera
		std::map<float, Entity::id_t> sorted;
		for (auto& kvPair : frameData.entities) {
			auto& obj = kvPair.second;
			if (!obj.hasComponent<PointLightComponent>() || !obj.hasComponent<TransformComponent>()) continue;

			// calculate distance
			const TransformComponent& transform = *obj.getComponent<TransformComponent>();
			auto offset = frameData.camera.getPosition() - transform.translation;
			float distanceSq = glm::dot(offset, offset);
			sorted[distanceSq] = obj.getId();
		}

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

		
		for (auto iter = sorted.rbegin(); iter != sorted.rend(); ++iter) {
			auto& obj = frameData.entities.at(iter->second);

			const TransformComponent& transform = *obj.getComponent<TransformComponent>();
			const PointLightComponent& light = *obj.getComponent<PointLightComponent>();

			PointLightPushConstants pushConstants{};
			pushConstants.position = glm::vec4(transform.translation, 1.0f);
			pushConstants.color = glm::vec4(light.color, light.intensity);
			pushConstants.radius = transform.scale.x;

			vkCmdPushConstants(
				frameData.cmdBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PointLightPushConstants),
				&pushConstants
			);
			vkCmdDraw(frameData.cmdBuffer, 6, 1, 0, 0);
		}
	}
}