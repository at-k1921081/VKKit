#ifndef DEFAULT_CONFIGURATIONS_H
#define DEFAULT_CONFIGURATIONS_H

#include "GraphicsPipeline.h"
#include "DescriptorSetLayout.h"
#include "vulkan/vulkan.h"

namespace VKKit {
class Device;
class RenderPass;
class Swapchain;
class DescriptorPool;
class Buffer;

GraphicsPipeline CreateColor2DPipeline(VkPhysicalDevice physical_device, const Device& device, const RenderPass& render_pass,
    const Swapchain& swapchain, const DescriptorSetLayout& dsl, VkSampleCountFlagBits msaa);
DescriptorSetLayout CreateColor2DLayout(const Device& device);

GraphicsPipeline CreateTexture2DPipeline(VkPhysicalDevice physical_device, const Device& device, const RenderPass& render_pass,
    const Swapchain& swapchain, const DescriptorSetLayout& dsl, VkSampleCountFlagBits msaa);
DescriptorSetLayout CreateTexture2DLayout(const Device& device);

GraphicsPipeline CreateColor3DPipeline(VkPhysicalDevice physical_device, const Device& device, const RenderPass& render_pass,
    const Swapchain& swapchain, const DescriptorSetLayout& dsl, VkSampleCountFlagBits msaa);
DescriptorSetLayout CreateColor3DLayout(const Device& device);
VkDescriptorSet CreateColor3DSet(const Device& device, const DescriptorPool& pool, const DescriptorSetLayout& dsl, std::span<const Buffer> uniform_buffers);

GraphicsPipeline CreateTexture3DPipeline(VkPhysicalDevice physical_device, const Device& device, const RenderPass& render_pass,
    const Swapchain& swapchain, const DescriptorSetLayout& dsl, VkSampleCountFlagBits msaa);
DescriptorSetLayout CreateTexture3DLayout(const Device& device);

GraphicsPipeline CreateTextPipeline(VkPhysicalDevice physical_device, const Device& device, const RenderPass& render_pass,
    const Swapchain& swapchain, const DescriptorSetLayout& dsl, VkSampleCountFlagBits msaa);
DescriptorSetLayout CreateTextLayout(const Device& device);
}

#endif