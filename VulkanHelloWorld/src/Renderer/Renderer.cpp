#include "Renderer.h"
#include <stdexcept>
#include <array>

Renderer::Renderer(Devices& device, SwapChain& swapchain, Camera& cam, const int maxFrame)
	:m_device(device), m_swapchain(swapchain), m_MAX_FRAMES_IN_FLIGHT(maxFrame), m_camera(cam)
{
	createSyncObjects();
	createCommandBuffers();
	createGlobalUniformBuffers();
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

void Renderer::beginRenderPass(VkCommandBuffer cmd, VkRenderPass renderPass, VkFramebuffer framebuffer, VkExtent2D extent)
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = extent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f,0 };

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
	m_gblUniformBuffers[m_currentFrame]->update(&ubo);
}



Renderer::~Renderer()
{
	for (size_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_device.getLogicalDevice(), m_renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_device.getLogicalDevice(), m_imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_device.getLogicalDevice(), m_inFlightFences[i], nullptr);
	}
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
	m_gblUniformVkBuffers.resize(m_MAX_FRAMES_IN_FLIGHT);
	m_gblUniformVkMemory.resize(m_MAX_FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < m_gblUniformBuffers.size(); i++)
	{
		m_gblUniformBuffers[i] = std::make_unique<UniformBuffer>(m_device, sizeof(GlobalUniformBufferObject));
		m_gblUniformVkBuffers[i] = m_gblUniformBuffers[i]->getHandle();
		m_gblUniformVkMemory[i] = m_gblUniformBuffers[i]->getMemory();
	}
}

