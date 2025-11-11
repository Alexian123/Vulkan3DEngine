#pragma once

#include "Camera.h"
#include "Constants.h"
#include "Entity.h"

#include <vulkan/vulkan.h>

namespace Vulkan3DEngine
{
	struct PointLight
	{
		glm::vec4 position{}; // ignore w
		glm::vec4 color{}; // w = intensity
	};

	struct GlobalUbo
	{
		glm::mat4 projectionMatrix{ 1.f };
		glm::mat4 viewMatrix{ 1.f };
		glm::mat4 invViewMatrix{ 1.f };
		glm::vec4 ambientLightColor{ 1.f, 1.f, 1.f, .02f };
		PointLight pointLights[AppConstants::MAX_LIGHTS];
		int numLights;
	};

	struct FrameData
	{
		int frameIndex;
		float frameTime;
		VkCommandBuffer cmdBuffer;
		Camera& camera;
		VkDescriptorSet globalDescSet;
		const EntityMap& entities;
	};

}