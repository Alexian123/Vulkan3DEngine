#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace Vulkan3DEngine
{

	class MathUtils
	{
	public:
		static glm::mat4 createTransformationMatrix(
			const glm::vec3& translation,
			const glm::vec3& rotation, // in radians
			const glm::vec3& scale
		);

		static glm::mat3 createNormalMatrix(
			const glm::vec3& rotation, // in radians
			const glm::vec3& scale
		);
	};

}