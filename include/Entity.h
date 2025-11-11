#pragma once

#include "Component.h"

#include <unordered_map>
#include <typeindex>
#include <memory>
#include <stdexcept>
#include <utility>

namespace Vulkan3DEngine
{
	class Entity
	{
	public:
		using id_t = unsigned int;
		using ComponentMap = std::unordered_map<std::type_index, std::shared_ptr<IComponent>>;

		Entity() = delete;
		Entity(id_t id) : id{ id } {}
		Entity(const Entity&) = delete;
		Entity& operator=(const Entity&) = delete;
		Entity(Entity&&) = default;
		Entity& operator=(Entity&&) = default;

	private:
		id_t id;
		ComponentMap components;

	public:
		id_t getId() const { return id; }

		template<typename T, typename... Args>
		void addComponent(Args&&... args)
		{
			using CompT = Component<T>;
			components[std::type_index(typeid(T))] =
				std::make_shared<CompT>(std::forward<Args>(args)...);
		}

		template<typename T>
		T* getComponent()
		{
			auto it = components.find(std::type_index(typeid(T)));
			if (it == components.end()) return nullptr;
			auto base = it->second.get();
			auto derived = dynamic_cast<Component<T>*>(base);
			return derived ? &derived->value : nullptr;
		}

		template<typename T>
		const T* getComponent() const
		{
			auto it = components.find(std::type_index(typeid(T)));
			if (it == components.end()) return nullptr;
			auto base = it->second.get();
			auto derived = dynamic_cast<Component<T>*>(base);
			return derived ? &derived->value : nullptr;
		}

		template<typename T>
		bool hasComponent() const
		{
			return components.find(std::type_index(typeid(T))) != components.end();
		}

		template<typename T>
		void removeComponent()
		{
			components.erase(std::type_index(typeid(T)));
		}

		const ComponentMap& getAllComponents() const { return components; }
	};

	// map alias
	using EntityMap = std::unordered_map<Entity::id_t, Entity>;
}