#include "Concurrency.h"
#include "VkResultString.h"

namespace VKKit {
Semaphore::Semaphore() : semaphore{ nullptr }, device{ nullptr }
{
}

Semaphore::Semaphore(VkDevice device) :
    device{ device }
{
    const VkSemaphoreCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    const auto result = vkCreateSemaphore(device, &info, nullptr, &semaphore);

    if (result != VK_SUCCESS) ThrowError("Failed to create semaphore.", result);
}

Semaphore::~Semaphore()
{
    if (device) vkDestroySemaphore(device, semaphore, nullptr);
}

Semaphore::Semaphore(Semaphore&& s) noexcept
{
    semaphore = s.semaphore;
    s.semaphore = nullptr;
    device = s.device;
}

Semaphore& Semaphore::operator=(Semaphore&& s) noexcept
{
    if (device) vkDestroySemaphore(device, semaphore, nullptr);
    semaphore = s.semaphore;
    s.semaphore = nullptr;
    device = s.device;
    return *this;
}

void Semaphore::Wait(uint64_t timeout) const
{
    const VkSemaphoreWaitInfo info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO
    };

    vkWaitSemaphores(device, &info, timeout);
}

Fence::Fence() : fence{ nullptr }, device{ nullptr }
{
}

Fence::Fence(VkDevice device, bool signaled) : device{ device }
{
    const VkFenceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : VkFenceCreateFlags{}
    };

    const auto result = vkCreateFence(device, &info, nullptr, &fence);
    if (result != VK_SUCCESS) ThrowError("Failed to create fence.", result);
}

Fence::~Fence()
{
    if (device) vkDestroyFence(device, fence, nullptr);
}

Fence::Fence(Fence&& f) noexcept
{
    fence = f.fence;
    f.fence = nullptr;
    device = f.device;
}

Fence& Fence::operator=(Fence&& f) noexcept
{
    if (device) vkDestroyFence(device, fence, nullptr);
    fence = f.fence;
    f.fence = nullptr;
    device = f.device;
    return *this;
}

void Fence::Wait(uint64_t timeout) const
{
    assert(device != nullptr);
    assert(fence != nullptr);
    vkWaitForFences(device, 1, &fence, VK_TRUE, timeout);
}

void Fence::Reset() const
{
    assert(device != nullptr);
    assert(fence != nullptr);
    vkResetFences(device, 1, &fence);
}
}