#pragma once

#include "DeviceManager.h"
#include "Model.h"

#include <string>
#include <vector>

namespace Vulkan3DEngine
{

	struct PipelineConfigInfo
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
		std::vector<VkVertexInputAttributeDescription> attribDescriptions{};
		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		std::vector<VkDynamicState> dynamicStateEnables;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo;
		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0;

		PipelineConfigInfo() = default;
		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;
	};

	class GfxPipeline
	{
	private:
		DeviceManager& deviceManager;
		VkPipeline pipeline;
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;

	public:
		static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
		static void enableAlphaBlending(PipelineConfigInfo& configInfo);

		GfxPipeline(
			DeviceManager& deviceManager,
			const std::string& vertShaderPath, 
			const std::string& fragShaderPath,
			const PipelineConfigInfo& configInfo
		);
		~GfxPipeline();

		GfxPipeline(const GfxPipeline&) = delete;
		GfxPipeline& operator=(const GfxPipeline&) = delete;

		void bind(VkCommandBuffer commandBuffer);

	private:
		void createGraphicsPipeline(
			const std::string& vertShaderPath, 
			const std::string& fragShaderPath,
			const PipelineConfigInfo& configInfo
		);

		void createShaderModule(const std::vector<char> codeBytes, VkShaderModule* shaderModule);
	};

}