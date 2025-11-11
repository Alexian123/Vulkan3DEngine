#pragma once

#include <memory>
#include <typeindex>

namespace Vulkan3DEngine
{
	struct IComponent
	{
		virtual ~IComponent() = default;
	};

	template<typename T>
	struct Component : IComponent
	{
		T value;
		template<typename... Args>
		Component(Args&&... args) : value(std::forward<Args>(args)...) {}
	};
}