#pragma once
#include<glm/glm.hpp>
#include<vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include<array>
#include "Core/Devices.h"

class Buffer
{
public:
	static void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	static void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	static void copyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
};


class CommandBuffer
{
public:
	static VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
	static void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkCommandBuffer commandBuffer);
};

class UniformBuffer
{
public:
	UniformBuffer(Devices& device, VkDeviceSize size);
	~UniformBuffer();

	UniformBuffer(const UniformBuffer&) = delete;
	UniformBuffer& operator=(const UniformBuffer&) = delete;

	VkBuffer getHandle() const { return m_buffer; }
	VkDeviceMemory getMemory() const { return m_buffermemory; }
	void update(const void* data);
private:
	Devices& m_device;
	VkDeviceSize m_size;
	VkBuffer m_buffer;
	VkDeviceMemory m_buffermemory;
	void* m_mappedData = nullptr;

};



