#pragma once
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <string>
#include <memory>
#include "../Core/Devices.h"

class Texture
{
public:
	Texture(Devices& device,
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImageAspectFlags aspectFlags);
	~Texture();

	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

	const VkImage& getImage() const { return m_image; }
	const VkImageView& getImageView() const { return m_imageView; }
	const VkFormat& getFormat() const { return m_format; }

	static std::shared_ptr<Texture> createPureColorTexture(Devices& device, uint32_t color);

	static std::shared_ptr<Texture> loadFromFile(Devices& device, const std::string& path);
	static std::unique_ptr<Texture> createDepthTexture(Devices& device, uint32_t width, uint32_t height, VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
private:
	Devices& m_device; // 引用Devices 类，方便获取物理和逻辑设备
	VkImage m_image = VK_NULL_HANDLE;
	VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
	VkImageView m_imageView = VK_NULL_HANDLE;
	VkFormat m_format;

	void createImage(uint32_t width, uint32_t height, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	void createImageView(VkImageAspectFlags aspectFlags);
	void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
