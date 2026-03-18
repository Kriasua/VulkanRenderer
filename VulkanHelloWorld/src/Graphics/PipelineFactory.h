#pragma once
#include <memory>
#include <string>
#include "PipelineBuilder.h"
#include "../Core/Devices.h"

class PipelineFactory
{
public:
	//核心函数：根据类型生产管线
	static std::shared_ptr<Pipeline> createStandardPipeline(Devices& device, VkRenderPass renderPass, VkExtent2D extent, VkDescriptorSetLayout layout);

	// 为以后的 NPR 预留接口
	static std::shared_ptr<Pipeline> createToonPipeline(Devices& device, VkRenderPass renderPass, VkExtent2D extent, VkDescriptorSetLayout layout);
	static std::shared_ptr<Pipeline> createOutlinePipeline(Devices& device, VkRenderPass renderPass, VkExtent2D extent, VkDescriptorSetLayout layout);

	//阴影贴图管线
	static std::shared_ptr<Pipeline> createShadowPipeline(Devices& device, VkRenderPass renderPass, VkDescriptorSetLayout layout);
};
