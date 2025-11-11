#pragma once

#include "WindowManager.h"
#include "DeviceManager.h"
#include "Renderer.h"
#include "Constants.h"
#include "Descriptors.h"
#include "Entity.h"
#include "EntityComponents.h"

#include <memory>
#include <vector>

namespace Vulkan3DEngine
{

	class AppController
	{
	private:
		WindowManager winManager{ AppConstants::DEFAULT_WINDOW_WIDTH, AppConstants::DEFAULT_WINDOW_HEIGHT, AppConstants::APP_NAME };
		DeviceManager devManager{ winManager };
		Renderer renderer{ winManager, devManager };

		std::unique_ptr<DescriptorPoolManager> globalPoolManager{};

		EntityMap entities;
		Entity::id_t nextEntityId = 1;

	public:
		AppController();
		~AppController();

		AppController(const AppController&) = delete;
		AppController& operator=(const AppController&) = delete;

		void run();

	private:
		void loadEntities();
	};

}