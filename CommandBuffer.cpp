#include "CommandBuffer.h"
#include "VkResultString.h"
#include "Device.h"
#include "CommandPool.h"

namespace VKKit {
CommandBuffer::CommandBuffer() noexcept : device{ nullptr }, pool{ nullptr }, buffer{ nullptr }
{}

CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBufferLevel level) :
    device{ device }, pool{ pool }
{
    const VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .level = level,
        .commandBufferCount = 1
    };

    const auto result = vkAllocateCommandBuffers(device, &alloc_info, &buffer);
    if (result != VK_SUCCESS) ThrowError("Failed to allocate command buffer.", result);
}

CommandBuffer::CommandBuffer(const Device& device, const CommandPool& pool, VkCommandBufferLevel level) :
    CommandBuffer(device.Get(), pool.Get(), level)
{}

CommandBuffer::~CommandBuffer()
{
    if (device) vkFreeCommandBuffers(device, pool, 1, &buffer);
}

CommandBuffer::CommandBuffer(CommandBuffer&& buf) noexcept :
    device{ buf.device }, pool{ buf.pool }, buffer{ buf.buffer }
{
    buf.device = nullptr;
    buf.pool = nullptr;
    buf.buffer = nullptr;
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& buf) noexcept
{
    if (device) vkFreeCommandBuffers(device, pool, 1, &buffer);

    device = buf.device;
    buf.device = nullptr;
    pool = buf.pool;
    buf.pool = nullptr;
    buffer = buf.buffer;
    buf.buffer = nullptr;

    return *this;
}

void CommandBuffer::Begin(VkCommandBufferUsageFlags flags) const
{
    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = flags
    };

    const auto result = vkBeginCommandBuffer(buffer, &begin_info);
    assert(result == VK_SUCCESS);
    (void)result;
}

void CommandBuffer::End() const
{
    const auto result = vkEndCommandBuffer(buffer);
    assert(result == VK_SUCCESS);
    (void)result;
}

void CommandBuffer::Submit(VkQueue queue, bool device_wait) const
{
    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &buffer
    };

    const auto submit = vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    // assert(submit == VK_SUCCESS);
    (void)submit;
    if (device_wait) {
        const auto wait = vkQueueWaitIdle(queue);
        // assert(wait == VK_SUCCESS);
        (void)wait;
    }
}
}