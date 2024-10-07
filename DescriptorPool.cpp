#include "DescriptorPool.h"
#include "VkResultString.h"
#include "Device.h"

namespace VKKit {
DescriptorPool::DescriptorPool() noexcept :
    device{ nullptr }, pool{ nullptr }
{}

DescriptorPool::DescriptorPool(VkDevice device, VkDescriptorPoolCreateFlags flags, std::span<const VkDescriptorPoolSize> pool_sizes, uint32_t frames_in_flight) :
    device{ device }
{
    uint32_t max_sets = 0;
    for (const auto ps : pool_sizes) max_sets += ps.descriptorCount;
    max_sets *= frames_in_flight;

    const VkDescriptorPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = flags,
        .maxSets = max_sets,
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data()
    };

    const auto result = vkCreateDescriptorPool(device, &info, nullptr, &pool);
    if (result != VK_SUCCESS) ThrowError("Failed to create descriptor pool.", result);
}

DescriptorPool::DescriptorPool(const Device& device, VkDescriptorPoolCreateFlags flags, std::span<const VkDescriptorPoolSize> pool_sizes, uint32_t frames_in_flight) :
    DescriptorPool(device.Get(), flags, pool_sizes, frames_in_flight)
{}

DescriptorPool::~DescriptorPool()
{
    if (device) vkDestroyDescriptorPool(device, pool, nullptr);
}

DescriptorPool::DescriptorPool(DescriptorPool&& dp) noexcept :
    device{ dp.device }, pool{ dp.pool }
{
    dp.device = nullptr;
    dp.pool = nullptr;
}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& dp) noexcept
{
    if (device) vkDestroyDescriptorPool(device, pool, nullptr);
    device = dp.device;
    pool = dp.pool;
    dp.device = nullptr;
    dp.pool = nullptr;
    return *this;
}

void DescriptorPool::Reset() noexcept
{
    const auto result = vkResetDescriptorPool(device, pool, {});
    (void)result;
    assert(result == VK_SUCCESS);
}
}