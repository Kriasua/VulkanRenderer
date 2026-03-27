#define STB_IMAGE_IMPLEMENTATION
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <array>
#include <set>
#include <stb_image.h>
#include "Core/ValidationLayerAssist.h"
#include <chrono>
#include "Buffer.h"
#include "Vertex.h"
#include "Description.h"
#include "Core/Devices.h"
#include "Renderer/Renderer.h"
#include "Scene/Scene.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Shader.h"
#include "Graphics/PipelineBuilder.h"
#include "Graphics/RenderPass.h"
#include "Graphics/Framebuffer.h"
#include "Graphics/Texture.h"
#include "Graphics/Camera.h"
#include "Graphics/Model.h"
#include "Graphics/Material.h"
#include "Graphics/Entity.h"
#include "Graphics/PipelineFactory.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

//初始为1080p，可直接拉动窗口边缘调节大小
const uint32_t WIDTH = 1920;
const uint32_t HEIGHT = 1080;
const int MAX_FRAMES_IN_FLIGHT = 3;

class HelloTriangleApplication
{
public:

	GLFWwindow* window;
	void run()
	{
		initWindow();
		m_device = std::make_unique<Devices>(window,MAX_FRAMES_IN_FLIGHT);
		m_swapChain = std::make_unique<SwapChain>(*m_device, windowExtent);
		m_renderer = std::make_unique<Renderer>(*m_device, &(*m_swapChain), m_camera, MAX_FRAMES_IN_FLIGHT);
		initVulkan();
		mainLoop();
		cleanUp();
	}

private:
	std::unique_ptr<Renderer> m_renderer;
	std::unique_ptr<Devices> m_device;
	std::unique_ptr<SwapChain> m_swapChain;

	std::unique_ptr<Scene> m_scene;

	VkExtent2D windowExtent;

	VkDescriptorSetLayout descriptorSetLayout;
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

	Camera m_camera{};
	float deltaTime;
	float lastFrame;

	// 追踪鼠标
	bool firstMouse = true;//防止鼠标刚移入窗口时计算错误

	//默认在窗口中心
	float lastX = WIDTH / 2.0f;
	float lastY = HEIGHT / 2.0f;

	bool framebufferResized = false;

	bool qKeyPressedLastFrame = false;

	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
		glfwSetCursorPosCallback(window, mouseCallback); // 绑定鼠标回调
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // 隐藏鼠标，沉浸式体验！
		windowExtent = { WIDTH, HEIGHT };
	}

	void initVulkan()
	{

		//创建模型，贴图（实体资源）
		m_scene = std::make_unique<Scene>(*m_device);
		m_scene->loadTexture("images/viking_room.png");//0号贴图
		m_scene->loadTexture(0xFFFFFFFF);//1号贴图
		m_scene->loadModel("models/VikingRoom/viking_room.obj");//0号模型
		m_scene->loadModel("models/plane/plane.obj");//1号模型

		//创建材质（虚拟资源）
		std::shared_ptr<Material> m_vikingRoomMat = std::make_shared<Material>(*m_device, m_swapChain->getSwapChainImages().size(), PipelineFactory::createStandardPipeline(
			*m_device, m_renderer->getRenderPass().getHandle(), m_swapChain->getSwapChainExtent(),
			Descriptor::createDescriptorSetLayout(m_device->getLogicalDevice())));
		m_vikingRoomMat->addTexture(1, m_scene->getTextures()[0], m_renderer->getLinearRepeatSampler());
		m_vikingRoomMat->addTexture(2, m_renderer->getshadowTexture(), m_renderer->getShadowSampler());
		m_vikingRoomMat->build(*m_renderer);

		std::shared_ptr<Material> m_PureColorMat = std::make_shared<Material>(*m_device, m_swapChain->getSwapChainImages().size(), PipelineFactory::createStandardPipeline(
			*m_device, m_renderer->getRenderPass().getHandle(), m_swapChain->getSwapChainExtent(),
			Descriptor::createDescriptorSetLayout(m_device->getLogicalDevice())));
		m_PureColorMat->addTexture(1, m_scene->getTextures()[1], m_renderer->getLinearRepeatSampler());
		m_PureColorMat->addTexture(2, m_renderer->getshadowTexture(), m_renderer->getShadowSampler());
		m_PureColorMat->build(*m_renderer);

		m_scene->addMaterial(m_vikingRoomMat);
		m_scene->addMaterial(m_PureColorMat);
		m_scene->addEntity(m_scene->getModels()[0], m_scene->getMaterials()[0]);
		std::unique_ptr<Entity> ground = std::make_unique<Entity>(m_scene->getModels()[1], m_scene->getMaterials()[1]);
		ground->setPosition(glm::vec3{ 0.0f,0.0f,-2.0f });
		m_scene->addEntity(std::move(ground));

		float offset = 2.0f;
		float sscale = 0.9f;
		for (int i = 0; i < 4; i++)
		{
			std::unique_ptr<Entity> m_vikingEntity2 = std::make_unique<Entity>(m_scene->getModels()[0], m_scene->getMaterials()[0]);
			m_vikingEntity2->setScale(glm::vec3{ sscale });
			m_vikingEntity2->setPosition(glm::vec3{ offset,0.0f,0.0f });
			m_scene->addEntity(std::move(m_vikingEntity2));
			offset += 2.0f;
			sscale -= 0.14f;
		}

		m_renderer->initImGui(window);
	}
	void processInput(GLFWwindow* window) {
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) m_camera.processKeyboard(FORWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) m_camera.processKeyboard(BACKWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) m_camera.processKeyboard(LEFT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) m_camera.processKeyboard(RIGHT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) m_camera.reset();

		bool qKeyPressedThisFrame = (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS);

		if (qKeyPressedThisFrame && !qKeyPressedLastFrame) {
			if (m_camera.active) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				m_camera.active = false;
			}
			else {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				m_camera.active = true;
			}
		}
		qKeyPressedLastFrame = qKeyPressedThisFrame;
	}

	//C语言api绑定回调函数只能静态，因为非静态有this指针，它不认识这个
	static void mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
		auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->handleMouse(xposIn, yposIn);
	}

	void handleMouse(double xposIn, double yposIn) {
		float xpos = static_cast<float>(xposIn);
		float ypos = static_cast<float>(yposIn);

		if (firstMouse) {
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = lastX - xpos;
		float yoffset = lastY - ypos; 
		lastX = xpos;
		lastY = ypos;

		m_camera.processMouseMovement(xoffset, yoffset);
	}

	void mainLoop()
	{
		auto lastTime = std::chrono::high_resolution_clock::now();
		int frameCount = 0;
		while (!glfwWindowShouldClose(window))
		{
			float currentFrame = static_cast<float>(glfwGetTime());
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;
			glfwPollEvents();
			processInput(window);
			// 1. ImGui 开启新帧
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			// 2. 控制窗口

			ImGui::Begin("Light Controller");
			ImGui::SliderFloat("Light Yaw", &m_renderer->m_lightYaw, 0.0f, 360.0f);
			ImGui::SliderFloat("Light Pitch", &m_renderer->m_lightPitch, -90.0f, 90.0f);
			// 增加一个重置按钮
			if (ImGui::Button("Reset Light")) {
				m_renderer->m_lightYaw = 45.0f;
				m_renderer->m_lightPitch = 45.0f;
			}
			ImGui::End();

			//3. 生成渲染数据
			ImGui::Render();
			drawFrame();

			auto currentTime = std::chrono::high_resolution_clock::now();
			frameCount++;
			float timeDiff = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();

			if (timeDiff >= 0.3f) {
				// 计算 FPS
				float fps = frameCount / timeDiff;
				int fpsinINT = static_cast<int>(fps);

				std::string title = "Vulkan - FPS: " + std::to_string(fpsinINT);
				glfwSetWindowTitle(window, title.c_str());

				// 重置
				lastTime = currentTime;
				frameCount = 0;
			}
		}

		vkDeviceWaitIdle(m_device->getLogicalDevice());
	}

	static void framebufferResizeCallback(GLFWwindow* window,int width, int height)
	{
		auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	void cleanUp()
	{
		vkDeviceWaitIdle(m_device->getLogicalDevice());
		m_renderer->cleanupSwapChainAssets();
		m_scene.reset();
		m_renderer.reset();
		m_swapChain.reset();
		m_device.reset();
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void drawFrame()
	{
		VkCommandBuffer cmd = m_renderer->beginFrame();
		if (cmd == VK_NULL_HANDLE) {
			recreateSwapChain();
			return;
		}

		m_renderer->updateGlbUBO();

		//开始阴影Renderpass
		m_renderer->beginRenderPass(cmd, m_renderer->getShadowRenderPass(), m_renderer->getShadowPassFrameBuffer()->getHandle(), {2048,2048});
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_renderer->getShadowPipeline()->getPipeline());
		VkDescriptorSet shadowSet = m_renderer->getShadowDescriptorSet(m_renderer->getFrameIndex());
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_renderer->getShadowPipeline()->getPipelineLayout().getHandle(), 0, 1, &shadowSet, 0, nullptr);
		m_scene->drawforShadow(cmd, m_renderer->getShadowPipeline()->getPipelineLayout().getHandle());
		m_renderer->endRenderPass(cmd);

		//开始场景渲染的主pass
		m_renderer->beginRenderPass(cmd, m_renderer->getRenderPass(), m_renderer->getFrameBuffers()[m_renderer->getImageIndex()]->getHandle(), m_swapChain->getSwapChainExtent());
		m_scene->drawMain(cmd, m_renderer->getFrameIndex());
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
		m_renderer->endRenderPass(cmd);
		VkResult result = m_renderer->endFrame();

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

	}


	void recreateSwapChain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_device->getLogicalDevice());

		m_swapChain.reset();
		m_renderer->cleanupSwapChainAssets();

		VkExtent2D newExtent = {width, height};
		m_swapChain = std::make_unique<SwapChain>(*m_device, newExtent);
		
		m_renderer->setSwapChain(&(*m_swapChain));
		m_renderer->createRenderPass();
		m_renderer->createDepthResource();
		m_renderer->createSwapchainFrameBuffers();
		std::cout << "Recreated Swapchain!" << std::endl;
	}

};


int main()
{
	HelloTriangleApplication app;
	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
