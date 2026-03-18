#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "Texture.h"
#include "../Core/Devices.h"
#include <map>
#include "../Renderer/Renderer.h"
#include "PipelineBuilder.h"

class Material
{
public:
	Material(Devices& device, int maxFrame, std::shared_ptr<Pipeline> m_pipeline);
	~Material();

	Material(const Material&) = delete;
	Material& operator=(const Material&) = delete;

	void addTexture(uint32_t binding, std::shared_ptr<Texture> texture, VkSampler sampler);
	void addUniformBuffer(uint32_t binding, const std::vector<VkBuffer>& buffers, VkDeviceSize range);

	void build(Renderer& renderer);
	void bind(VkCommandBuffer cmdbuf, uint32_t currentFrame);
	
	void setPipeline(std::shared_ptr<Pipeline> pipeline)
	{ 
		m_pipeline = pipeline; 
		m_deslayout = m_pipeline->getDescriptorSetLayout();
	}

	const std::shared_ptr<Pipeline>& getPipeline() const {
		return m_pipeline;
	}

private:
	Devices& m_device;
	int m_MAX_FRAMES_IN_FLIGHT;
	std::shared_ptr<Pipeline> m_pipeline;
	std::vector<VkDescriptorSet> m_descriptorSets;
	VkDescriptorSetLayout m_deslayout = VK_NULL_HANDLE;
	int m_descriptorCounts = 1;
	struct TextureData
	{
		uint32_t binding;
		std::shared_ptr<Texture> tex;
		VkSampler sampler;
	};

	struct UniformData
	{
		uint32_t binding;
		std::vector<VkBuffer> buffers;
		VkDeviceSize range;
	};

	std::map<uint32_t, TextureData> m_textures;
	std::map<uint32_t, UniformData> m_uniformBuffers;

};


