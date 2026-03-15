#include "Material.h"


Material::Material(Devices& device, int maxFrame, VkDescriptorSetLayout& deslayout)
	: m_device(device), m_MAX_FRAMES_IN_FLIGHT(maxFrame), m_deslayout(deslayout)
{

}

void Material::addTexture(uint32_t binding, std::shared_ptr<Texture> texture)
{
	TextureData data = { binding,texture };
	m_textures.emplace(binding, data);
}

void Material::addUniformBuffer(uint32_t binding, const std::vector<VkBuffer>& buffers, VkDeviceSize range)
{
	UniformData data = { binding,buffers,range };
	m_uniformBuffers.emplace(binding, data);
}

void Material::build(Renderer& renderer, VkSampler sampler)
{
	std::vector<VkDescriptorSetLayout> layouts(m_MAX_FRAMES_IN_FLIGHT, m_deslayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_device.getDescriptorPool();
	allocInfo.descriptorSetCount = static_cast<uint32_t>(m_MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	m_descriptorSets.resize(m_MAX_FRAMES_IN_FLIGHT);

	if (vkAllocateDescriptorSets(m_device.getLogicalDevice(), &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}


	for (size_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; i++)
	{
		std::vector<VkWriteDescriptorSet> descriptorWrites;

		std::vector<VkDescriptorBufferInfo> bufferInfos;
		std::vector<VkDescriptorImageInfo> imageInfos;
		bufferInfos.reserve(m_uniformBuffers.size());
		imageInfos.reserve(m_textures.size());

		//加载全局global uniform
		VkDescriptorBufferInfo globalBufferInfo{};
		globalBufferInfo.buffer = renderer.getGlbUniformBuffers()[i]->getHandle();
		globalBufferInfo.offset = 0;
		globalBufferInfo.range = sizeof(GlobalUniformBufferObject);


		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = nullptr;
		descriptorWrite.pBufferInfo = &globalBufferInfo; // Optional
		descriptorWrite.pTexelBufferView = nullptr; // Optional

		descriptorWrites.push_back(descriptorWrite);

		for (auto& key : m_uniformBuffers)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = key.second.buffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = key.second.range;
			bufferInfos.push_back(bufferInfo);

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_descriptorSets[i];
			descriptorWrite.dstBinding = key.second.binding;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pImageInfo = nullptr;
			descriptorWrite.pBufferInfo = &bufferInfos.back(); // Optional
			descriptorWrite.pTexelBufferView = nullptr; // Optional

			descriptorWrites.push_back(descriptorWrite);
		}

		for (auto& key : m_textures)
		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = key.second.tex->getImageView();
			imageInfo.sampler = sampler;
			imageInfos.push_back(imageInfo);

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_descriptorSets[i];
			descriptorWrite.dstBinding = key.second.binding;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pImageInfo = &imageInfos.back();
			descriptorWrite.pBufferInfo = nullptr; // Optional
			descriptorWrite.pTexelBufferView = nullptr; // Optional

			descriptorWrites.push_back(descriptorWrite);
		}

		vkUpdateDescriptorSets(m_device.getLogicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	}

}

void Material::bind(VkCommandBuffer cmdbuf, VkPipelineLayout layout, uint32_t currentFrame)
{
	vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->getPipeline());
	vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &m_descriptorSets[currentFrame], 0, nullptr);
}

Material::~Material()
{
	if (!m_descriptorSets.empty()) {
		// 把这几帧占用的 Sets 还给 Devices 里的全局池
		vkFreeDescriptorSets(
			m_device.getLogicalDevice(),
			m_device.getDescriptorPool(),
			static_cast<uint32_t>(m_descriptorSets.size()),
			m_descriptorSets.data()
		);
	}
}

