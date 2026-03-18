#include "PipelineFactory.h"
#include "Shader.h"
#include "../Vertex.h"
#include <stdexcept>

std::shared_ptr<Pipeline> PipelineFactory::createStandardPipeline(Devices& device, VkRenderPass renderPass, VkExtent2D extent, VkDescriptorSetLayout descripLayout)
{
	/////////////////////////////////////////////////////////////////////////////////////
		////////////////////// 图形管线可编程阶段的配置(shaders) /////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////

	Shader vertShader(device.getLogicalDevice(), "shader/vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	Shader fragShader(device.getLogicalDevice(), "shader/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertShader.getStageInfo(), fragShader.getStageInfo() };

	/////////////////////////////////////////////////////////////////////////////////////
	////////////////////// 图形管线其他阶段的配置 (固定功能阶段) /////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////


	VertexLayout layout;
	layout.push<glm::vec3>();//位置
	layout.push<glm::vec3>();//颜色
	layout.push<glm::vec2>();//UV
	layout.push<glm::vec3>();//法线

	PipelineBuilder builder;
	builder.shaderStages.push_back(vertShader.getStageInfo());
	builder.shaderStages.push_back(fragShader.getStageInfo());
	builder.setVertexInput(layout.getBindingDescription(), layout.getAttributeDescriptions());
	builder.viewport = { 0.0f,0.0f,(float)extent.width ,(float)extent.height ,0.0f,1.0f };
	builder.scissor = { {0,0}, extent };
	builder.enableDepthTest();

	std::vector<VkDescriptorSetLayout> layouts = { descripLayout };
	auto pipelineLayout = std::make_unique<PipelineLayout>(device.getLogicalDevice(), layouts);
	builder.setPipelineLayout(pipelineLayout->getHandle());

	VkPipeline rawPipeline = builder.build(device.getLogicalDevice(), renderPass);
	if (rawPipeline == VK_NULL_HANDLE) {
		throw std::runtime_error("Failed to create graphics pipeline!");
	}
	std::shared_ptr<Pipeline> pipeline = std::make_shared<Pipeline>(device.getLogicalDevice(), rawPipeline);
	pipeline->setPipelineLayout(std::move(pipelineLayout));
	pipeline->setDescriptorSetLayout(descripLayout);
	return pipeline;
}

	std::shared_ptr<Pipeline> PipelineFactory::createShadowPipeline(Devices& device, VkRenderPass renderPass, VkDescriptorSetLayout descripLayout)
	{
		Shader vertShader(device.getLogicalDevice(), "shader/shadowVert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertShader.getStageInfo()};
	
		PipelineBuilder builder;
		builder.shaderStages = { vertShader.getStageInfo() };

		VertexLayout layout;
		layout.push<glm::vec3>();//位置
		layout.push<glm::vec3>();//颜色
		layout.push<glm::vec2>();//UV
		layout.push<glm::vec3>();//法线

		// 2. 顶点输入
		builder.setVertexInput(layout.getBindingDescription(), layout.getAttributeDescriptions());
	
		VkExtent2D shadowExtent = { 2048, 2048 };
		builder.viewport = { 0.0f, 0.0f, (float)shadowExtent.width, (float)shadowExtent.height, 0.0f, 1.0f };
		builder.scissor = { {0, 0}, shadowExtent };

		// 3. 图元装配
		builder.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		// 4. 光栅化器设置 (🌟 区别二 & 三：剔除正面 + 开启深度偏移)
		builder.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		// 强烈建议在算阴影时剔除正面 (FRONT_BIT)，只画背面。这能极大缓解“漏光”和“彼得潘效应”
		builder.rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
		builder.rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		// 开启 Depth Bias (解决阴影粉刺 Shadow Acne 的终极武器)
		builder.rasterizer.depthBiasEnable = VK_TRUE;
		builder.rasterizer.depthBiasConstantFactor = 1.25f; // 基础偏移
		builder.rasterizer.depthBiasClamp = 0.0f;
		builder.rasterizer.depthBiasSlopeFactor = 1.75f;    // 根据斜率增加的偏移
		builder.rasterizer.lineWidth = 1.0f;

		// 5. 多重采样 (阴影图通常不需要 MSAA)
		builder.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		builder.multisampling.sampleShadingEnable = VK_FALSE;

		// 6. 深度测试设置
		builder.enableDepthTest(); // 调用你封装好的函数开启深度读写
		builder.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		// 7. 颜色混合设置 (🌟 区别四：因为 RenderPass 里没有颜色坑位，这里必须彻底置空！)
		builder.colorBlending.logicOpEnable = VK_FALSE;
		builder.colorBlending.attachmentCount = 0;       // 极其关键：告诉管线不输出任何颜色
		builder.colorBlending.pAttachments = nullptr;    // 极其关键：清空附件指针

		// 8. 创建 Pipeline Layout (绑定 UBO 和未来的贴图)
		std::vector<VkDescriptorSetLayout> layouts = { descripLayout };
		auto pipelineLayout = std::make_unique<PipelineLayout>(device.getLogicalDevice(), layouts);
		builder.setPipelineLayout(pipelineLayout->getHandle());

		// 9. 呼叫 Builder 生成底层的 VkPipeline
		VkPipeline rawPipeline = builder.build(device.getLogicalDevice(), renderPass);

		// 10. 打包进你的 Pipeline 包装类并返回
		auto pipeline = std::make_shared<Pipeline>(device.getLogicalDevice(), rawPipeline);
		pipeline->setPipelineLayout(std::move(pipelineLayout));
		pipeline->setDescriptorSetLayout(descripLayout);

		return pipeline;
	}

