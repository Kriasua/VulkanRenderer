#include "Renderer.h"
#include <stdexcept>
#include <array>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include "../Buffer.h"

Renderer::Renderer(Devices& device, SwapChain& swapchain, Camera& cam, const int maxFrame)
	:m_device(device), m_swapchain(swapchain), m_MAX_FRAMES_IN_FLIGHT(maxFrame), m_camera(cam)
{
	createRenderPass();
	createSyncObjects();
	createCommandBuffers();
	createGlobalUniformBuffers();
	createSamplers();
	createDepthResource();
	createSwapchainFrameBuffers();
	createShadowMapFramebuffers();
}

void Renderer::createRenderPass()
{
	//////主renderpass
	m_RenderPass = std::make_unique<RenderPass>(m_device.getLogicalDevice());

	AttachmentConfig colorAttachment = {};
	colorAttachment.format = m_swapchain.getSwapChainImageFormat();
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // 渲染完用于显示
	colorAttachment.clearValue.color = { {0.0f,0.0f,0.0f,1.0f} };
	m_RenderPass->addAttachment(colorAttachment);

	AttachmentConfig depthAttachment = {};
	depthAttachment.format = VK_FORMAT_D32_SFLOAT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // 渲染完深度数据就可以丢弃了
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.clearValue.depthStencil = { 1.0f, 0 };
	m_RenderPass->addAttachment(depthAttachment);

	// 3. 配置子流程 (对应原来的 VkSubpassDescription 和 Reference)
	SubpassConfig subpass = {};
	// 我们刚才加的颜色附件索引是 0，告诉子流程往 0 号坑位画
	subpass.colorAttachmentIndices = { 0 };
	subpass.depthAttachmentIndex = 1;
	m_RenderPass->addSubpass(subpass);

	DependencyConfig dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

	m_RenderPass->addDependency(dependency);
	m_RenderPass->create();

	//创建shadow map的renderpass
	m_shadowRenderPass = std::make_unique<RenderPass>(m_device.getLogicalDevice());
	depthAttachment = {};
	depthAttachment.format = VK_FORMAT_D32_SFLOAT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	depthAttachment.clearValue.depthStencil = { 1.0f,0 };
	m_shadowRenderPass->addAttachment(depthAttachment);

	subpass = {};
	subpass.depthAttachmentIndex = 0;
	m_shadowRenderPass->addSubpass(subpass);

	dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	m_shadowRenderPass->addDependency(dependency);
	m_shadowRenderPass->create();
}

VkCommandBuffer Renderer::beginFrame()
{
	vkWaitForFences(m_device.getLogicalDevice(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_device.getLogicalDevice(), m_swapchain.getSwapChain(), std::numeric_limits<uint64_t>::max(), m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return VK_NULL_HANDLE;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	vkResetFences(m_device.getLogicalDevice(), 1, &m_inFlightFences[m_currentFrame]);
	m_imageIndex = imageIndex;
	// D. 开启 CommandBuffer 录制
	VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
	vkResetCommandBuffer(cmd, 0); // 擦干净之前的记录

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording command buffer!");
	}
	return cmd;

}

VkResult Renderer::endFrame()
{
	VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
	if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	std::array<VkSemaphore, 1> waitSemaphores = { m_imageAvailableSemaphores[m_currentFrame] };
	std::array<VkPipelineStageFlags, 1> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages.data();

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	std::array<VkSemaphore, 1> signalSemaphores = { m_renderFinishedSemaphores[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores.data();
	if (vkQueueSubmit(m_device.getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores.data();
	std::array<VkSwapchainKHR, 1> swapChains = { m_swapchain.getSwapChain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains.data();
	presentInfo.pImageIndices = &m_imageIndex;
	presentInfo.pResults = nullptr;
	VkResult result = vkQueuePresentKHR(m_device.getPresentQueue(), &presentInfo);
	m_currentFrame = (m_currentFrame + 1) % m_MAX_FRAMES_IN_FLIGHT;

	return result;
}

void Renderer::beginRenderPass(VkCommandBuffer cmd, RenderPass& renderPass, VkFramebuffer framebuffer, VkExtent2D extent)
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass.getHandle();
	renderPassInfo.framebuffer = framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = extent;

	const auto& clearValues = renderPass.getClearValues();
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	//开始录制
	vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	// 配置并设置动态视口 (Viewport)
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f; // 深度范围：近平面
	viewport.maxDepth = 1.0f; // 深度范围：远平面
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	// 配置并设置动态裁剪矩形 (Scissor)
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = extent;
	vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void Renderer::endRenderPass(VkCommandBuffer cmd)
{
	vkCmdEndRenderPass(cmd);
}

void Renderer::updateGlbUBO()
{
	GlobalUniformBufferObject ubo = {};
	ubo.view = m_camera.getViewMatrix();
	ubo.proj = m_camera.getProjectionMatrix(m_swapchain.getSwapChainExtent().width / (float)m_swapchain.getSwapChainExtent().height);
	ubo.time = static_cast<float>(glfwGetTime());

	float yawRad = glm::radians(m_lightYaw);
	float pitchRad = glm::radians(m_lightPitch);

	glm::vec3 dir;
	dir.x = cos(pitchRad) * cos(yawRad);
	dir.y = cos(pitchRad) * sin(yawRad);
	dir.z = sin(pitchRad);
	dir = glm::normalize(dir);

	// 计算旋转后的方向并归一化
	ubo.lightDir = glm::vec4(dir, 0.0f);
	ubo.lightColor = m_lightColor; 

	//生成光源出深度贴图的VP矩阵
	// a.设定正交投影范围(根据你平面 14.14 的半径，设为 16.0 留点余量)
	float sceneRadius = 16.0f;
	// 参数: left, right, bottom, top, zNear, zFar
	glm::mat4 lightProjection = glm::ortho(-sceneRadius, sceneRadius, -sceneRadius, sceneRadius, 1.0f, 60.0f);

	// 翻转轴
	lightProjection[1][1] *= -1;

	glm::vec3 sceneCenter = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 lightPos = sceneCenter - dir * 30.0f;
	glm::vec3 upVector = glm::vec3(0.0f, 0.0f, 1.0f); 

	// 【安全处理】：如果光线从正上方垂直照下来 (dir.z 接近 1 或 -1)，
	// dir 会与 upVector 平行，导致 lookAt 计算出 NaN（画面黑屏崩溃）。
	// 所以当接近垂直时，临时用 Y 轴做 up 向量。
	if (abs(dir.z) > 0.99f) {
		upVector = glm::vec3(0.0f, 1.0f, 0.0f);
	}

	glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, upVector);
	ubo.lightMat = lightProjection * lightView;
	m_gblUniformBuffers[m_currentFrame]->update(&ubo);
}



Renderer::~Renderer()
{
	vkDeviceWaitIdle(m_device.getLogicalDevice());
	for (size_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_device.getLogicalDevice(), m_renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_device.getLogicalDevice(), m_imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_device.getLogicalDevice(), m_inFlightFences[i], nullptr);
	}

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	if (m_imguiPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(m_device.getLogicalDevice(), m_imguiPool, nullptr);
	}

	vkDestroySampler(m_device.getLogicalDevice(), m_UISampler, nullptr);
	vkDestroySampler(m_device.getLogicalDevice(), m_ShadowSampler, nullptr);
	vkDestroySampler(m_device.getLogicalDevice(), m_NearestRepeatSampler, nullptr);
	vkDestroySampler(m_device.getLogicalDevice(), m_LinearRepeatSampler, nullptr);
}

void Renderer::createSyncObjects()
{
	size_t n = m_swapchain.getSwapChainImageViews().size();
	m_imageAvailableSemaphores.resize(n);
	m_renderFinishedSemaphores.resize(n);
	m_inFlightFences.resize(n);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	for (size_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(m_device.getLogicalDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_device.getLogicalDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphores!");
		}
	}

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (size_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateFence(m_device.getLogicalDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create fences!");
		}
	}
}

void Renderer::createCommandBuffers()
{
	m_commandBuffers.resize(m_MAX_FRAMES_IN_FLIGHT);

	//从commandPool里面申请内存给commandBuffers
	//有几个framebuffer就申请几个commandBuffer
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_device.getCommandPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
	if (vkAllocateCommandBuffers(m_device.getLogicalDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void Renderer::createGlobalUniformBuffers()
{
	m_gblUniformBuffers.resize(m_MAX_FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < m_gblUniformBuffers.size(); i++)
	{
		m_gblUniformBuffers[i] = std::make_unique<UniformBuffer>(m_device, sizeof(GlobalUniformBufferObject));
		
	}
}

void Renderer::createSamplers()
{
	VkSamplerCreateInfo LinearRepeatSamplerInfo = {};
	LinearRepeatSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	LinearRepeatSamplerInfo.magFilter = VK_FILTER_LINEAR;
	LinearRepeatSamplerInfo.minFilter = VK_FILTER_LINEAR;
	LinearRepeatSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	LinearRepeatSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	LinearRepeatSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	LinearRepeatSamplerInfo.anisotropyEnable = VK_TRUE;
	LinearRepeatSamplerInfo.maxAnisotropy = 16;
	LinearRepeatSamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	LinearRepeatSamplerInfo.unnormalizedCoordinates = VK_FALSE;
	LinearRepeatSamplerInfo.compareEnable = VK_FALSE;
	LinearRepeatSamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	LinearRepeatSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	LinearRepeatSamplerInfo.mipLodBias = 0.0f;
	LinearRepeatSamplerInfo.minLod = 0.0f;
	LinearRepeatSamplerInfo.maxLod = 0.0f;
	if (vkCreateSampler(m_device.getLogicalDevice(), &LinearRepeatSamplerInfo, nullptr, &m_LinearRepeatSampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Linear Repeat Sampler!");
	}

	// --- 2. Nearest Repeat Sampler (用于 NPR 或像素风格，色块感强) ---
	VkSamplerCreateInfo nearestInfo = LinearRepeatSamplerInfo; // 复制基础配置
	nearestInfo.magFilter = VK_FILTER_NEAREST;
	nearestInfo.minFilter = VK_FILTER_NEAREST;
	nearestInfo.anisotropyEnable = VK_FALSE; // 像素风通常不需要各向异性

	if (vkCreateSampler(m_device.getLogicalDevice(), &nearestInfo, nullptr, &m_NearestRepeatSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create nearest sampler!");
	}

	// --- 3. Shadow Sampler (专门给阴影贴图用，硬件级 PCF 开启) ---
	VkSamplerCreateInfo shadowInfo{};
	shadowInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	shadowInfo.magFilter = VK_FILTER_LINEAR;
	shadowInfo.minFilter = VK_FILTER_LINEAR;
	// 阴影贴图边缘通常要 Clamp，防止阴影在远处重复
	shadowInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	shadowInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	shadowInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	shadowInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE; // 边界外认为是“亮的”

	//核心：开启深度比较
	shadowInfo.compareEnable = VK_TRUE;
	shadowInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	if (vkCreateSampler(m_device.getLogicalDevice(), &shadowInfo, nullptr, &m_ShadowSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shadow sampler!");
	}

	// --- 4. UI Sampler (用于图标和 UI，不重复，不拉伸) ---
	VkSamplerCreateInfo uiInfo = LinearRepeatSamplerInfo;
	uiInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	uiInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	uiInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	uiInfo.anisotropyEnable = VK_FALSE; // UI 没必要开各项异性

	if (vkCreateSampler(m_device.getLogicalDevice(), &uiInfo, nullptr, &m_UISampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create UI sampler!");
	}
}

void Renderer::createShadowMapFramebuffers()
{
	VkExtent2D extent = { 2048,2048 };
	std::vector<VkImageView> attachs = { m_shadowDepthTex->getImageView() };
	m_shadowPassframebuffer = std::make_unique<Framebuffer>(m_device.getLogicalDevice(),
			m_shadowRenderPass->getHandle(), extent, attachs);
}

void Renderer::createSwapchainFrameBuffers()
{
	const std::vector<VkImageView>& swapchainImageViews = m_swapchain.getSwapChainImageViews();
	m_framebuffers.clear();
	m_framebuffers.reserve(swapchainImageViews.size());

	for (size_t i = 0; i < swapchainImageViews.size(); i++)
	{
		std::vector<VkImageView> attachs = { swapchainImageViews[i], m_depthTex->getImageView() };

		m_framebuffers.push_back(std::make_unique<Framebuffer>(m_device.getLogicalDevice(),
			m_RenderPass->getHandle(), m_swapchain.getSwapChainExtent(), attachs));
	}
}

void Renderer::cleanupSwapChainAssets()
{
	m_framebuffers.clear();
	m_depthTex.reset();
	if (m_RenderPass) {
		m_RenderPass.reset();
	}
}

void Renderer::createDepthResource() {
	m_depthTex = Texture::createDepthTexture(
		m_device,
		m_swapchain.getSwapChainExtent().width,
		m_swapchain.getSwapChainExtent().height
	);

	m_shadowDepthTex = Texture::createDepthTexture(m_device,2048,2048,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
}

void Renderer::initImGui(GLFWwindow* window)
{
	// 1. 创建描述符池 (保持不变)
	VkDescriptorPoolSize pool_sizes[] = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
	};
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1;
	pool_info.poolSizeCount = 1;
	pool_info.pPoolSizes = pool_sizes;

	if (vkCreateDescriptorPool(m_device.getLogicalDevice(), &pool_info, nullptr, &m_imguiPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create imgui descriptor pool");
	}

	// 2. 初始化上下文
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForVulkan(window, true);

	// 3. 初始化 Vulkan 后端
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.ApiVersion = VK_API_VERSION_1_3;
	init_info.Instance = m_device.getInstance();
	init_info.PhysicalDevice = m_device.getPhysicalDevice();
	init_info.Device = m_device.getLogicalDevice();

	// 获取队列族索引
	auto indices = m_device.getQueueFamilyIndices(m_device.getPhysicalDevice());
	init_info.QueueFamily = indices.graphicsFamily.value();
	init_info.Queue = m_device.getGraphicsQueue();

	init_info.DescriptorPool = m_imguiPool;
	init_info.MinImageCount = m_MAX_FRAMES_IN_FLIGHT;
	init_info.ImageCount = m_MAX_FRAMES_IN_FLIGHT;

	// 设置 Pipeline 参数
	init_info.PipelineInfoMain.RenderPass = m_RenderPass->getHandle();
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	
	ImGui_ImplVulkan_Init(&init_info);
}

