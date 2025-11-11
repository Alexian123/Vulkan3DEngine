#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace Vulkan3DEngine
{

	class WindowManager
	{
	private:
		int width;
		int height;
		bool windowResized = false;

		std::string windowTitle;

		GLFWwindow* window;

	public:
		WindowManager(int width, int height, std::string windowTitle);
		~WindowManager();

		WindowManager(const WindowManager&) = delete;
		WindowManager& operator=(const WindowManager&) = delete;

		GLFWwindow* getWindow() const;

		bool windowShouldClose() const;
		
		bool windowWasResized() const;
		void resetWindowResizedFlag();

		VkExtent2D getExtent();
		void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

	private:
		static void windowResizedCallback(GLFWwindow* window, int width, int height);

		void initWindow();
	};

}