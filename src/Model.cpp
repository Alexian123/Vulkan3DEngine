#include "Model.h"

#include "HashUtils.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <cassert>
#include <cstring>
#include <unordered_map>

namespace std
{
	template<>
	struct hash<Vulkan3DEngine::Model::Vertex>
	{
		size_t operator()(const Vulkan3DEngine::Model::Vertex& vertex) const
		{
			size_t seed = 0;
			Vulkan3DEngine::HashUtils::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.texCoords);
			return seed;
		}
	};
}

namespace Vulkan3DEngine
{
	std::vector<VkVertexInputBindingDescription> Model::Vertex::getBindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);

		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attribDescriptions{};

		// { location, binding, format, offset }
		attribDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) });
		attribDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) });
		attribDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
		attribDescriptions.push_back({ 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoords) });
		
		return attribDescriptions;
	}

	bool Model::Vertex::operator==(const Vertex& other) const
	{
		return this->position == other.position && this->color == other.color &&
			this->normal == other.normal && this->texCoords == other.texCoords;
	}

	void Model::Data::load(const std::string& objPath)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath.c_str())) {
			throw std::runtime_error(warn + err);
		}

		vertices.clear();
		indices.clear();

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};
		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vert{};

				if (index.vertex_index >= 0) {
					vert.position = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2],
					};

					vert.color = {
						attrib.colors[3 * index.vertex_index + 0],
						attrib.colors[3 * index.vertex_index + 1],
						attrib.colors[3 * index.vertex_index + 2],
					};
				}

				if (index.normal_index >= 0) {
					vert.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2],
					};
				}

				if (index.texcoord_index >= 0) {
					vert.texCoords = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						attrib.texcoords[2 * index.texcoord_index + 1],
					};
				}

				if (uniqueVertices.count(vert) == 0) {
					uniqueVertices[vert] = static_cast<uint32_t>(vertices.size()); // position in vertices vector from Model::Data
					vertices.push_back(vert);
				}
				indices.push_back(uniqueVertices[vert]);
			}
		}
	}

	std::unique_ptr<Model> Model::createModelFromFile(DeviceManager& devManager, const std::string& filePath)
	{
		Data modelData{};
		modelData.load(filePath);
		return std::make_unique<Model>(devManager, modelData);
	}

	Model::Model(DeviceManager& deviceManager, const Model::Data& modelData) : deviceManager{ deviceManager }
	{
		createVertexBuffers(modelData.vertices);
		createIndexBuffer(modelData.indices);
	}

	Model::~Model()
	{
	}

	void Model::bind(VkCommandBuffer commandBuffer)
	{
		VkBuffer buffers[] = { vertBufferManager->getBuffer()};
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

		if (hasIndexBuffer) {
			vkCmdBindIndexBuffer(commandBuffer, idxBufferManager->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		}
	}

	void Model::draw(VkCommandBuffer commandBuffer)
	{
		if (hasIndexBuffer) {
			vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
		}
		else {
			vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
		}
	}

	void Model::createVertexBuffers(const std::vector<Vertex>& vertices)
	{
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3");
		uint32_t sizeOfVertex = sizeof(vertices[0]);

		BufferManager stagingBufferManager{
			deviceManager,
			sizeOfVertex,
			vertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		stagingBufferManager.map();
		stagingBufferManager.writeToBuffer((void*)vertices.data());

		vertBufferManager = std::make_unique<BufferManager>(
			deviceManager,
			sizeOfVertex,
			vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		VkDeviceSize bufferSize = sizeOfVertex * vertexCount;
		deviceManager.copyBuffer(stagingBufferManager.getBuffer(), vertBufferManager->getBuffer(), bufferSize);
	}

	void Model::createIndexBuffer(const std::vector<uint32_t>& indices)
	{
		indexCount = static_cast<uint32_t>(indices.size());
		
		hasIndexBuffer = indexCount > 0;
		if (!hasIndexBuffer) {
			return;
		}

		uint32_t sizeOfIndex = sizeof(indices[0]);
		VkDeviceSize bufferSize = sizeOfIndex * indexCount;

		BufferManager stagingBufferManager{
			deviceManager,
			sizeOfIndex,
			indexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		stagingBufferManager.map();
		stagingBufferManager.writeToBuffer((void*)indices.data());

		idxBufferManager = std::make_unique<BufferManager>(
			deviceManager,
			sizeOfIndex,
			indexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		deviceManager.copyBuffer(stagingBufferManager.getBuffer(), idxBufferManager->getBuffer(), bufferSize);
	}
	
}