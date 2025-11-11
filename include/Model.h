#pragma once

#include "DeviceManager.h"
#include "BufferManager.h"
#include "MathUtils.h"

#include <memory>
#include <vector>

namespace Vulkan3DEngine
{

	class Model
	{
	public:
		struct Vertex
		{
			glm::vec3 position{};
			glm::vec3 color{};
			glm::vec3 normal{};
			glm::vec2 texCoords{};

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

			bool operator==(const Vertex& other) const;
		};

		struct Data
		{
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};

			void load(const std::string& objPath);
		};

	private:
		DeviceManager& deviceManager;

		std::unique_ptr<BufferManager> vertBufferManager;
		uint32_t vertexCount;

		bool hasIndexBuffer = false;
		std::unique_ptr<BufferManager> idxBufferManager;
		uint32_t indexCount;

	public:
		static std::unique_ptr<Model> createModelFromFile(DeviceManager& devManager, const std::string& filePath);

		Model(DeviceManager& deviceManager, const Model::Data& modelData);
		~Model();

		Model(const Model&) = delete;
		Model& operator=(const Model&) = delete;

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);

	private:
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		void createIndexBuffer(const std::vector<uint32_t>& indices);
	};

}