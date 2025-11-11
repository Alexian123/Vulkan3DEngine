#include "AppController.h"

#include "SimpleRenderSystem.h"
#include "PointLightRenderSystem.h"
#include "FrameInfo.h"
#include "Camera.h"
#include "CameraMovementHandler.h"
#include "BufferManager.h"
#include "Entity.h"
#include "EntityComponents.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <array>
#include <chrono>
#include <numeric>

namespace Vulkan3DEngine
{
	AppController::AppController()
	{
		globalPoolManager = DescriptorPoolManager::Builder(devManager)
			.setMaxSets(AppConstants::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, AppConstants::MAX_FRAMES_IN_FLIGHT)
			.build();
		loadEntities();
	}

	AppController::~AppController()
	{
		globalPoolManager = nullptr;
	}

	void AppController::run()
	{
		std::vector<std::unique_ptr<BufferManager>> uboManagers(AppConstants::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uboManagers.size(); ++i) {
			uboManagers[i] = std::make_unique<BufferManager>(
				devManager,
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
			uboManagers[i]->map();
		}

		auto globalSetLayoutManager = DescriptorSetLayoutManager::Builder(devManager)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.build();

		std::vector<VkDescriptorSet> globalDescriptorSets(AppConstants::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); ++i) {
			auto bufferInfo = uboManagers[i]->descriptorInfo();
			DescriptorWriter(*globalSetLayoutManager, *globalPoolManager)
				.writeBuffer(0, &bufferInfo)
				.build(globalDescriptorSets[i]);
		}

		auto simpleRenderSystem = SimpleRenderSystem::create(
			devManager, 
			renderer.getSwapChainRenderPass(), 
			globalSetLayoutManager->getDescriptorSetLayout()
		);

		auto pointLightRenderSystem = PointLightRenderSystem::create(
			devManager,
			renderer.getSwapChainRenderPass(),
			globalSetLayoutManager->getDescriptorSetLayout()
		);
		
		Camera camera{};
		camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

		Entity::id_t viewerId = nextEntityId;
		Entity viewer = Entity(nextEntityId++);
		viewer.addComponent<TransformComponent>();
		// start viewer further back on Z
		if (auto t = viewer.getComponent<TransformComponent>()) {
			t->translation.z = -2.5f;
		}
		entities.emplace(viewer.getId(), std::move(viewer));

		CameraMovementHandler camMovementHandler{};

		auto time1 = std::chrono::high_resolution_clock::now();

		while (!winManager.windowShouldClose()) {
			glfwPollEvents();

			auto time2 = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(time2 - time1).count();
			time1 = time2;

			// Drive camera using the viewer entity's TransformComponent
			auto& viewerEntity = entities.at(viewerId);
			auto* viewerTransform = viewerEntity.getComponent<TransformComponent>();
			if (viewerTransform) {
				camMovementHandler.moveInPlaneXZ(winManager.getWindow(), frameTime, *viewerTransform);
				camera.setViewYXZ(viewerTransform->translation, viewerTransform->rotation);
			}

			float aspectRatio = renderer.getAspectRatio();
			camera.setPerspectiveProjection(glm::radians(50.f), aspectRatio, 0.1f, 1000.0f);

			if (auto cmdBuffer = renderer.beginFrame()) {
				int frameIndex = renderer.getCurrentFrameIndex();
				FrameData frameData{
					frameIndex,
					frameTime,
					cmdBuffer,
					camera,
					globalDescriptorSets[frameIndex],
					entities
				};

				// update
				GlobalUbo ubo{};
				ubo.projectionMatrix = camera.getProjectionMatrix();
				ubo.viewMatrix = camera.getViewMatrix();
				ubo.invViewMatrix = camera.getInverseViewMatrix();
				pointLightRenderSystem->update(frameData, ubo);
				uboManagers[frameIndex]->writeToBuffer(&ubo);
				uboManagers[frameIndex]->flush();

				// render
				renderer.beginSwapChainRenderPass(cmdBuffer);
				simpleRenderSystem->render(frameData);
				pointLightRenderSystem->render(frameData);
				renderer.endSwapChainRenderPass(cmdBuffer);
				renderer.endFrame();
			}

			//vkDeviceWaitIdle(devManager.getDeviceHandle()); // fix for begin command buffer validation error on nvidia gpu
		}
		vkDeviceWaitIdle(devManager.getDeviceHandle());
	}

	void AppController::loadEntities()
	{
		// simplified: load models once and create entities with Model+Transform components
		std::shared_ptr<Model> smoothVase = Model::createModelFromFile(devManager, "resources/models/smooth_vase.obj");
		std::shared_ptr<Model> flatVase = Model::createModelFromFile(devManager, "resources/models/flat_vase.obj");
		std::shared_ptr<Model> quad = Model::createModelFromFile(devManager, "resources/models/quad.obj");

		// smooth vase entity
		{
			Entity e(nextEntityId++);
			e.addComponent<TransformComponent>();
			auto* t = e.getComponent<TransformComponent>();
			t->translation = { -0.5f, .5f, 0.f };
			t->scale = { 3.f, 1.5f, 3.f };

			e.addComponent<ModelComponent>();
			auto* m = e.getComponent<ModelComponent>();
			m->model = smoothVase;

			entities.emplace(e.getId(), std::move(e));
		}

		// flat vase entity
		{
			Entity e(nextEntityId++);
			e.addComponent<TransformComponent>();
			auto* t = e.getComponent<TransformComponent>();
			t->translation = { 0.5f, .5f, 0.f };
			t->scale = { 3.f, 1.5f, 3.f };

			e.addComponent<ModelComponent>();
			auto* m = e.getComponent<ModelComponent>();
			m->model = flatVase;

			entities.emplace(e.getId(), std::move(e));
		}

		// floor quad
		{
			Entity e(nextEntityId++);
			e.addComponent<TransformComponent>();
			auto* t = e.getComponent<TransformComponent>();
			t->translation = { 0.f, .5f, 0.f };
			t->scale = { 3.f, 1.f, 3.f };

			e.addComponent<ModelComponent>();
			auto* m = e.getComponent<ModelComponent>();
			m->model = quad;

			entities.emplace(e.getId(), std::move(e));
		}

		// point lights
		std::vector<glm::vec3> lightColors{
			{1.f, .1f, .1f},
			{.1f, .1f, 1.f},
			{.1f, 1.f, .1f},
			{1.f, 1.f, .1f},
			{.1f, 1.f, 1.f},
			{1.f, 1.f, 1.f}
		};

		for (size_t i = 0; i < lightColors.size(); ++i) {
			Entity e(nextEntityId++);
			e.addComponent<TransformComponent>();
			auto* t = e.getComponent<TransformComponent>();
			// rotate into position (same math as previous code)
			auto rotateLight = glm::rotate(
				glm::mat4(1.0f),
				(i * glm::two_pi<float>()) / static_cast<float>(lightColors.size()),
				glm::vec3(0.f, -1.f, 0.f)
			);
			t->translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
			t->scale.x = 0.1f;

			e.addComponent<PointLightComponent>();
			auto* pl = e.getComponent<PointLightComponent>();
			pl->intensity = 0.5f;
			pl->color = lightColors[i];

			entities.emplace(e.getId(), std::move(e));
		}
	}
}