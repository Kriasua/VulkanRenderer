#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include<glm/glm.hpp>
#include<vector>
#include "Graphics/Camera.h"


class Descriptor
{
public:
	static void createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayout);
	static void createDescriptorPool(VkDevice device, size_t swapChainImagesSize, VkDescriptorPool& descriptorPool);
};