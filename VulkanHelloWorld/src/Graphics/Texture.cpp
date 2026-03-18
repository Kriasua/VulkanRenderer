#include "Texture.h"
#include <stb_image.h>
#include "../Buffer.h"       

Texture::Texture(Devices& device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags)
	:m_device(device), m_format(format)
{
	createImage(width, height, tiling, usage, properties);
	createImageView(aspectFlags);
}

std::shared_ptr<Texture> Texture::createPureColorTexture(Devices& device, uint32_t color)
{
	VkDeviceSize imageSize = 4; // 1像素 * 4通道

	// 2. 创建 Staging Buffer 并拷贝数据
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	Buffer::createBuffer(
		device.getLogicalDevice(),
		device.getPhysicalDevice(),
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	void* data;
	vkMapMemory(device.getLogicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, &color, 4);
	vkUnmapMemory(device.getLogicalDevice(), stagingBufferMemory);

	// 3. 实例化 Texture (1x1)
	std::shared_ptr<Texture> texture = std::make_shared<Texture>(
		device,
		1, 1, // 宽高
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	// 4. 转换布局并拷贝
	texture->transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	Buffer::copyBufferToImage(
		device.getLogicalDevice(),
		device.getCommandPool(),
		device.getGraphicsQueue(),
		stagingBuffer,
		texture->getImage(),
		1, 1
	);

	texture->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	
	vkDestroyBuffer(device.getLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(device.getLogicalDevice(), stagingBufferMemory, nullptr);

	return texture;
}

std::shared_ptr<Texture> Texture::loadFromFile(Devices& device, const std::string& path)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels)
	{
		throw std::runtime_error("failed to load texture image!");
	}
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	Buffer::createBuffer(device.getLogicalDevice(), device.getPhysicalDevice(), imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	void* data;
	vkMapMemory(device.getLogicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device.getLogicalDevice(), stagingBufferMemory);
	stbi_image_free(pixels);

	std::shared_ptr<Texture> texture = std::make_unique<Texture>(device, texWidth, texHeight,
		VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);


	texture->transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	Buffer::copyBufferToImage(device.getLogicalDevice(), device.getCommandPool(), device.getGraphicsQueue(), stagingBuffer, texture->getImage(), static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	texture->transitionImageLayout( VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(device.getLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(device.getLogicalDevice(), stagingBufferMemory, nullptr);

	return texture;
}

std::unique_ptr<Texture> Texture::createDepthTexture(Devices& device, uint32_t width, uint32_t height, VkImageUsageFlags usage)
{
	return std::make_unique<Texture>(
		device,
		width,
		height,
		VK_FORMAT_D32_SFLOAT,                             // 强制 32 位浮点深度格式
		VK_IMAGE_TILING_OPTIMAL,
		usage,      // 专门作为深度附件
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT                         // 深度视角
	);
}

void Texture::createImage(uint32_t width, uint32_t height, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = m_format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateImage(m_device.getLogicalDevice(), &imageInfo, nullptr, &m_image) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_device.getLogicalDevice(), m_image, &memRequirements);
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
	if (vkAllocateMemory(m_device.getLogicalDevice(), &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate image memory!");
	}
	vkBindImageMemory(m_device.getLogicalDevice(), m_image, m_imageMemory, 0);
}

void Texture::createImageView(VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = m_format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	if (vkCreateImageView(m_device.getLogicalDevice(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture image view!");
	}
}

void Texture::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = CommandBuffer::beginSingleTimeCommands(m_device.getLogicalDevice(), m_device.getCommandPool());
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	//这里可以传递队列族所有权，但VK_QUEUE_FAMILY_IGNORED表示不做处理
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0; // TODO
	barrier.dstAccessMask = 0; // TODO

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	CommandBuffer::endSingleTimeCommands(m_device.getLogicalDevice(), m_device.getCommandPool(), m_device.getGraphicsQueue(), commandBuffer);
}

uint32_t Texture::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_device.getPhysicalDevice(), &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
}

Texture::~Texture()
{
	if (m_imageView != VK_NULL_HANDLE) {
		vkDestroyImageView(m_device.getLogicalDevice(), m_imageView, nullptr);
	}
	if (m_image != VK_NULL_HANDLE) {
		vkDestroyImage(m_device.getLogicalDevice(), m_image, nullptr);
	}
	if (m_imageMemory != VK_NULL_HANDLE) {
		vkFreeMemory(m_device.getLogicalDevice(), m_imageMemory, nullptr);
	}
}
