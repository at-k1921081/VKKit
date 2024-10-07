#include "DescriptorSetLayout.h"
#include "VkResultString.h"
#include "Device.h"

namespace VKKit {
DescriptorSetLayout::DescriptorSetLayout() noexcept :
    device{ nullptr }, layout{ nullptr }
{}

DescriptorSetLayout::DescriptorSetLayout(VkDevice device, std::span<const VkDescriptorSetLayoutBinding> bindings) :
    device{ device }
{
    const VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    const auto result = vkCreateDescriptorSetLayout(device, &info, nullptr, &layout);
    if (result != VK_SUCCESS) ThrowError("Failed to create descriptor set layout.", result);
}

DescriptorSetLayout::DescriptorSetLayout(const Device& device, std::span<const VkDescriptorSetLayoutBinding> bindings) :
    DescriptorSetLayout(device.Get(), bindings)
{}

DescriptorSetLayout::~DescriptorSetLayout()
{
    if (device) vkDestroyDescriptorSetLayout(device, layout, nullptr);
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& dsl) noexcept :
    device{ dsl.device }, layout{ dsl.layout }
{
    dsl.device = nullptr;
    dsl.layout = nullptr;
}

DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& dsl) noexcept
{
    if (device) vkDestroyDescriptorSetLayout(device, layout, nullptr);
    device = dsl.device;
    layout = dsl.layout;
    dsl.device = nullptr;
    dsl.layout = nullptr;
    return *this;
}
}