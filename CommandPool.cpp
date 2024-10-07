#include "CommandPool.h"
#include "VkResultString.h"
#include "Device.h"

namespace VKKit {
CommandPool::CommandPool() noexcept :
    device{ nullptr }, pool{ nullptr }
{}

CommandPool::CommandPool(VkDevice device, VkCommandPoolCreateFlags flags, uint32_t queue_family) :
    device{ device }
{
    const VkCommandPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = flags,
        .queueFamilyIndex = queue_family
    };

    const auto result = vkCreateCommandPool(device, &info, nullptr, &pool);
    if (result != VK_SUCCESS) ThrowError("Failed to create command pool.", result);
}

CommandPool::CommandPool(const Device& device, VkCommandPoolCreateFlags flags, uint32_t queue_family) :
    CommandPool(device.Get(), flags, queue_family)
{}

CommandPool::~CommandPool()
{
    if (device) vkDestroyCommandPool(device, pool, nullptr);
}

CommandPool::CommandPool(CommandPool&& cp) noexcept :
    device{ cp.device }, pool{ cp.pool }
{
    cp.device = nullptr;
    cp.pool = nullptr;
}

CommandPool& CommandPool::operator=(CommandPool&& cp) noexcept
{
    if (device) vkDestroyCommandPool(device, pool, nullptr);
    device = cp.device;
    pool = cp.pool;
    cp.device = nullptr;
    cp.pool = nullptr;
    return *this;
}
}