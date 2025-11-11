#include "WindowManager.h"

#include <stdexcept>

namespace Vulkan3DEngine
{
	WindowManager::WindowManager(int width, int height, std::string windowTitle)
		: width(width), height(height), windowTitle(windowTitle)
	{
		initWindow();
	}

	WindowManager::~WindowManager()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	GLFWwindow* WindowManager::getWindow() const
	{
		return window;
	}

	bool WindowManager::windowShouldClose() const
	{
		return glfwWindowShouldClose(window);
	}

	bool WindowManager::windowWasResized() const
	{
		return windowResized;
	}

	void WindowManager::resetWindowResizedFlag()
	{
		windowResized = false;
	}

	VkExtent2D WindowManager::getExtent()
	{
		return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	}

	void WindowManager::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
	{
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create window surface");
		}

	}

	void WindowManager::windowResizedCallback(GLFWwindow* window, int width, int height)
	{
		auto windowManager = reinterpret_cast<WindowManager*>(glfwGetWindowUserPointer(window));
		windowManager->windowResized = true;
		windowManager->width = width;
		windowManager->height = height;
	}

	void WindowManager::initWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		window = glfwCreateWindow(width, height, windowTitle.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, windowResizedCallback);
	}
}