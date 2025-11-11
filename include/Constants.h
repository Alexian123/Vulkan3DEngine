#pragma once

namespace Vulkan3DEngine
{
	struct AppConstants
	{
		static constexpr const char* APP_NAME = "Vulkan3DEngine App";
		static constexpr int APP_MAJOR_VERSION = 1;
		static constexpr int APP_MINOR_VERSION = 0;
		static constexpr int APP_PATCH_VERSION = 0;

		static constexpr const char* ENGINE_NAME = "Vulkan3DEngine";
		static constexpr int ENGINE_MAJOR_VERSION = 1;
		static constexpr int ENGINE_MINOR_VERSION = 0;
		static constexpr int ENGINE_PATCH_VERSION = 0;

		static constexpr int DEFAULT_WINDOW_WIDTH = 800;
		static constexpr int DEFAULT_WINDOW_HEIGHT = 600;

		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

		static constexpr int MAX_LIGHTS = 10;
	};
}