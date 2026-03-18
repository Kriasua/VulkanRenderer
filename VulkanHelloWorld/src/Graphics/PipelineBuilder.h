#pragma once
#include<vector>
#include <vulkan/vulkan.h>
#include <memory>

class PipelineBuilder
{
public:
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkViewport viewport;
	VkRect2D scissor;
	VkPipelineViewportStateCreateInfo viewportState;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineMultisampleStateCreateInfo multisampling;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo colorBlending;
	VkGraphicsPipelineCreateInfo pipelineInfo;
	VkPipelineLayout pipelineLayout;
	VkVertexInputBindingDescription _bindingDescription{};
	std::vector<VkVertexInputAttributeDescription> _attributeDescriptions{};
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	//////////////////////////////////////////////////////////////////////////////

	PipelineBuilder();
	VkPipeline build(VkDevice device, VkRenderPass pass);
	void setVertexInput(const VkVertexInputBindingDescription& binding,
		const std::vector<VkVertexInputAttributeDescription>& attributes);
	PipelineBuilder& setPipelineLayout(VkPipelineLayout layout)
	{
		this->pipelineLayout = layout;
		return *this;
	}

	void enableDepthTest();
	void disableDynamicState() {
		m_dynamicStates.clear();
	}

private:
	VkExtent2D m_SwapChainExtent;
	std::vector<VkDynamicState> m_dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
	};
};

class PipelineLayout
{
public:
	PipelineLayout(const PipelineLayout&) = delete;
	PipelineLayout& operator=(const PipelineLayout&) = delete;

	PipelineLayout(const VkDevice device, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
	~PipelineLayout();
	const VkPipelineLayout& getHandle() const { return m_layout; }
private:
	VkDevice m_device;
	VkPipelineLayout m_layout = VK_NULL_HANDLE;
};

class Pipeline
{
public:
	Pipeline(const Pipeline&) = delete;
	Pipeline& operator=(const Pipeline&) = delete;

	Pipeline(VkDevice device, VkPipeline pipeline);
	~Pipeline();

	VkPipeline& getPipeline() { return m_graphicsPipeline; }
	void setPipelineLayout(std::unique_ptr<PipelineLayout> layout) { m_pipelineLayout = std::move(layout); }
	void setDescriptorSetLayout(VkDescriptorSetLayout layout) { m_deslayout = layout; }

	VkDescriptorSetLayout getDescriptorSetLayout() const { return m_deslayout; }
	PipelineLayout& getPipelineLayout() const { return *m_pipelineLayout; }
private:
	VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	std::unique_ptr<PipelineLayout> m_pipelineLayout;
	VkDescriptorSetLayout m_deslayout = VK_NULL_HANDLE;
};
