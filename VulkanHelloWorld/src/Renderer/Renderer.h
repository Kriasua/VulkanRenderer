#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "../Core/Devices.h"
#include "../Graphics/Swapchain.h"
#include "../Buffer.h"
#include "../Graphics/Camera.h"

struct GlobalUniformBufferObject {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(4) float time;
};

class Renderer
{
public:
	Renderer(Devices& device, SwapChain& swapchain, Camera& cam, int maxFrame);
	~Renderer();
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	VkCommandBuffer beginFrame();
	VkResult endFrame();
	void beginRenderPass(VkCommandBuffer cmd, VkRenderPass renderPass, VkFramebuffer framebuffer, VkExtent2D extent);
	void endRenderPass(VkCommandBuffer cmd);

	void updateGlbUBO();

	uint32_t getImageIndex() const { return m_imageIndex; }
	size_t getFrameIndex() const { return m_currentFrame; }

	std::vector<VkCommandBuffer> m_commandBuffers;

	std::vector<std::unique_ptr<UniformBuffer>>& getGlbUniformBuffers() { return m_gblUniformBuffers; }
	std::vector<VkBuffer>& getGlbUni()  { return m_gblUniformVkBuffers; }
	std::vector<VkDeviceMemory>& getGlbUniMemo() { return m_gblUniformVkMemory; }
private:
	const int m_MAX_FRAMES_IN_FLIGHT;
	size_t m_currentFrame = 0;
	uint32_t m_imageIndex = 0;

	Devices& m_device;
	SwapChain& m_swapchain;
	Camera& m_camera;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;
	
	std::vector<std::unique_ptr<UniformBuffer>> m_gblUniformBuffers;
	std::vector<VkBuffer> m_gblUniformVkBuffers;
	std::vector<VkDeviceMemory> m_gblUniformVkMemory;

	void createSyncObjects();
	void createCommandBuffers();
	void createGlobalUniformBuffers();
};
