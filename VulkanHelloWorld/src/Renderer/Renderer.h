#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "../Core/Devices.h"
#include "../Graphics/Swapchain.h"
#include "../Buffer.h"
#include "../Graphics/Camera.h"
#include "../Graphics/RenderPass.h"
#include "../Graphics/Framebuffer.h"
#include "../Graphics/Texture.h"
#include "../Graphics/PipelineBuilder.h"

struct GlobalUniformBufferObject {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec4 lightDir;   
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::mat4 lightMat;
	alignas(16) float time;
};

class Renderer
{
public:
	Renderer(Devices& device, SwapChain* swapchain, Camera& cam, int maxFrame);
	~Renderer();
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	float m_lightYaw = 45.0f;    
	float m_lightPitch = 45.0f;  
	glm::vec4 m_lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	VkCommandBuffer beginFrame();
	VkResult endFrame();
	void beginRenderPass(VkCommandBuffer cmd, RenderPass& renderPass, VkFramebuffer framebuffer, VkExtent2D extent);
	void endRenderPass(VkCommandBuffer cmd);
	void updateGlbUBO();
	void createRenderPass();
	void createSwapchainFrameBuffers();
	void cleanupSwapChainAssets();
	void createDepthResource();
	void initImGui(GLFWwindow* window);

	uint32_t getImageIndex() const { return m_imageIndex; }
	size_t getFrameIndex() const { return m_currentFrame; }
	Texture& getDepthTex() const { return *m_depthTex; }
	const VkSampler getLinearRepeatSampler() const { return m_LinearRepeatSampler; };  // 通用默认
	const VkSampler getNearestRepeatSampler() const { return m_NearestRepeatSampler; }; // 用于 NPR 或像素风
	const VkSampler getShadowSampler() const { return m_ShadowSampler; };        // 专门给阴影贴图用
	const VkSampler getUISampler() const { return m_UISampler; };
	std::vector<std::unique_ptr<Framebuffer>>& getFrameBuffers() { return m_framebuffers; }
	RenderPass& getRenderPass() { return *m_RenderPass; }
	RenderPass& getShadowRenderPass() { return *m_shadowRenderPass; }
	std::vector<std::unique_ptr<UniformBuffer>>& getGlbUniformBuffers() { return m_gblUniformBuffers; }
	const std::shared_ptr<Texture> getshadowTexture() const { return m_shadowDepthTex; }
	const std::shared_ptr<Pipeline> getShadowPipeline() const { return m_shadowPipeline; }
	VkDescriptorSet getShadowDescriptorSet(uint32_t frameIndex) { return m_shadowDescriptorSets[frameIndex]; }
	const std::unique_ptr<Framebuffer>& getShadowPassFrameBuffer() const { return m_shadowPassframebuffer; }

	void setSwapChain(SwapChain* swapchain) { m_swapchain = swapchain; }
private:
	const int m_MAX_FRAMES_IN_FLIGHT;
	size_t m_currentFrame = 0;
	uint32_t m_imageIndex = 0;
	std::vector<VkCommandBuffer> m_commandBuffers;
	Devices& m_device;
	SwapChain* m_swapchain;
	Camera& m_camera;
	std::shared_ptr<Pipeline> m_shadowPipeline;
	VkDescriptorSetLayout m_shadowDescriptorSetLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> m_shadowDescriptorSets;
	VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;

	std::shared_ptr<Texture> m_depthTex;
	std::shared_ptr<Texture> m_shadowDepthTex;

	VkSampler m_LinearRepeatSampler = VK_NULL_HANDLE;
	VkSampler m_NearestRepeatSampler = VK_NULL_HANDLE;
	VkSampler m_ShadowSampler = VK_NULL_HANDLE;
	VkSampler m_UISampler = VK_NULL_HANDLE;

	std::unique_ptr<RenderPass> m_RenderPass;
	std::unique_ptr<RenderPass> m_shadowRenderPass;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;
	
	std::vector<std::unique_ptr<UniformBuffer>> m_gblUniformBuffers;
	std::vector<std::unique_ptr<Framebuffer>> m_framebuffers;
	std::unique_ptr<Framebuffer> m_shadowPassframebuffer;
	
	void createSyncObjects();
	void createCommandBuffers();
	void createGlobalUniformBuffers();
	void createSamplers();
	void createShadowMapFramebuffers();
	void createShadowDepthResources();
};
