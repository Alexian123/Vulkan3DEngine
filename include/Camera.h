#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace Vulkan3DEngine
{

	class Camera
	{
	private:
		glm::mat4 projectionMatrix{ 1.f };
		glm::mat4 viewMatrix{ 1.f };
		glm::mat4 inverseViewMatrix{ 1.f };

	public:
		void setOrtographicProjection(float left, float right, float top, float bottom, float near, float far);
		void setPerspectiveProjection(float fovY, float aspect, float near, float far);

		void setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3{ 0.f, -1.f, 0.f });
		void setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{ 0.f, -1.f, 0.f });
		void setViewYXZ(glm::vec3 position, glm::vec3 rotation);

		const glm::mat4& getProjectionMatrix() const;
		const glm::mat4& getViewMatrix() const;
		const glm::mat4& getInverseViewMatrix() const;
		glm::vec3 getPosition() const;
	};

}