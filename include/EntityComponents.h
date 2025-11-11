#pragma once

#include "Model.h"

#include <memory>

namespace Vulkan3DEngine
{
	struct TransformComponent
	{
		glm::vec3 translation{};
		glm::vec3 scale{ 1.f, 1.f, 1.f };
		glm::vec3 rotation{};
	};

	struct ModelComponent
	{
		std::shared_ptr<Model> model;
	};

	struct PointLightComponent
	{
		float intensity = 1.0f;
		glm::vec3 color{ 1.0f };
	};
}