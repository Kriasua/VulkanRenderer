#pragma once
#include <vulkan/vulkan.h>
#include <vector>

struct AttachmentConfig
{
	VkFormat format;
	VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkClearValue clearValue;
};

struct DependencyConfig
{
	uint32_t srcSubpass = VK_SUBPASS_EXTERNAL;
	uint32_t dstSubpass = 0;
	VkPipelineStageFlags srcStageMask;
	VkPipelineStageFlags dstStageMask;
	VkAccessFlags srcAccessMask;
	VkAccessFlags dstAccessMask;
};

struct SubpassConfig {
	VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	std::vector<uint32_t> colorAttachmentIndices;  // 往哪些颜色附件里写
	int depthAttachmentIndex = -1;                 // 往哪个深度附件里写（-1 表示不用）
	std::vector<uint32_t> inputAttachmentIndices;  // 从哪些附件里读（Subpass 间通信的核心）
};

class RenderPass
{
public:
	RenderPass(VkDevice device);
	~RenderPass();

	RenderPass(const RenderPass&) = delete;
	RenderPass& operator=(const RenderPass&) = delete;

	void addAttachment(const AttachmentConfig& config);
	void addDependency(const DependencyConfig& config);
	void addSubpass(const SubpassConfig& config);
	void create();

	const VkRenderPass& getHandle() const { return m_renderpass; }
	const std::vector<VkClearValue>& getClearValues() const { return m_clearValues; }
private:
	VkDevice m_device;
	std::vector<VkAttachmentDescription> m_descriptions;
	std::vector<SubpassConfig> m_configs;
	std::vector<VkSubpassDependency> m_dependencies;
	std::vector<VkClearValue> m_clearValues;
	VkRenderPass m_renderpass;
};
