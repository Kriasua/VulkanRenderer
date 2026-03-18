#include "RenderPass.h"
#include <stdexcept>
RenderPass::RenderPass(VkDevice device) : m_device(device), m_renderpass(VK_NULL_HANDLE)
{

}

void RenderPass::addAttachment(const AttachmentConfig& config)
{
	VkAttachmentDescription attach = {};
	attach.format = config.format;
	attach.samples = VK_SAMPLE_COUNT_1_BIT;
	attach.loadOp = config.loadOp;
	attach.storeOp = config.storeOp;
	attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attach.initialLayout = config.initialLayout;
	attach.finalLayout = config.finalLayout;
	m_descriptions.push_back(attach);

	m_clearValues.push_back(config.clearValue);
}

void RenderPass::addDependency(const DependencyConfig& config)
{
	VkSubpassDependency depend = {};
	depend.srcSubpass = config.srcSubpass;
	depend.dstSubpass = config.dstSubpass;
	depend.srcAccessMask = config.srcAccessMask;
	depend.dstAccessMask = config.dstAccessMask;
	depend.srcStageMask = config.srcStageMask;
	depend.dstStageMask = config.dstStageMask;
	depend.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;//Ä¬ÈÏÖµ
	m_dependencies.push_back(depend);
}

void RenderPass::addSubpass(const SubpassConfig& config)
{
	m_configs.push_back(config);
}

void RenderPass::create()
{
	std::vector<VkSubpassDescription> descs;

	std::vector<std::vector<VkAttachmentReference>> colorRefs;
	std::vector<std::vector<VkAttachmentReference>> inputRefs;
	std::vector<VkAttachmentReference> depthRefs;

	colorRefs.reserve(m_configs.size());
	inputRefs.reserve(m_configs.size());
	depthRefs.resize(m_configs.size());
	descs.reserve(m_configs.size());

	for (size_t i = 0; i < m_configs.size(); i++)
	{
		const SubpassConfig& config = m_configs[i];
		std::vector<VkAttachmentReference> colorRef;
		for (uint32_t idx : config.colorAttachmentIndices)
		{
			VkAttachmentReference ref = { idx,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
			colorRef.push_back(ref);
		}
		colorRefs.push_back(colorRef);

		std::vector<VkAttachmentReference> inputRef;
		for (uint32_t idx : config.inputAttachmentIndices)
		{
			VkAttachmentReference ref = { idx,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
			inputRef.push_back(ref);
		}
		inputRefs.push_back(inputRef);

		VkSubpassDescription subpass = {};
		subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs[i].size());
		subpass.inputAttachmentCount = static_cast<uint32_t>(inputRefs[i].size());
		subpass.pColorAttachments = colorRefs.empty()? nullptr : colorRefs[i].data();
		subpass.pInputAttachments = inputRefs.empty() ? nullptr : inputRefs[i].data();
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		

		if (config.depthAttachmentIndex != -1)
		{
			VkAttachmentReference depthRef = { config.depthAttachmentIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
			depthRefs[i] = depthRef;
			subpass.pDepthStencilAttachment = &depthRefs[i];
		}
		else
		{
			subpass.pDepthStencilAttachment = nullptr;
		}

		descs.push_back(subpass);
	}

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(m_descriptions.size());
	renderPassInfo.dependencyCount = static_cast<uint32_t>(m_dependencies.size());
	renderPassInfo.subpassCount = static_cast<uint32_t>(descs.size());
	renderPassInfo.pAttachments = m_descriptions.data();
	renderPassInfo.pDependencies = m_dependencies.data();
	renderPassInfo.pSubpasses = descs.data();


	if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderpass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}
}

RenderPass::~RenderPass()
{
	if (m_renderpass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(m_device, m_renderpass, nullptr);
	}
}

