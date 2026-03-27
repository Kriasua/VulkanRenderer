#pragma once
#include "../Core/Devices.h"

class SwapChain
{
public:
	SwapChain(Devices& deviceRef, VkExtent2D windowExtent);
	~SwapChain();
	SwapChain(const SwapChain&) = delete;
	SwapChain& operator=(const SwapChain&) = delete;

	VkSwapchainKHR getSwapChain() const { return m_swapChain; }
	VkFormat getSwapChainImageFormat() const { return m_swapChainImageFormat; }
	VkExtent2D getSwapChainExtent() const { return m_windowExtent; }
	const std::vector<VkImage>& getSwapChainImages() const { return m_swapChainImages; }
	const std::vector<VkImageView>& getSwapChainImageViews() const { return m_swapChainImageViews; }
	//const std::vector<VkFramebuffer>& getSwapChainFramebuffers() const { return m_swapChainFramebuffers; }
	//void createFramebuffers(VkRenderPass renderPass);

private:
	Devices& m_device;
	VkExtent2D m_windowExtent;
	VkSwapchainKHR m_swapChain;
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;
	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
	//std::vector<VkFramebuffer> m_swapChainFramebuffers;

	void createSwapChain();
	void createImageViews();
	VkImageView createImageView(VkImage image, VkFormat format);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
};