#ifndef COMMANDBUFFER_H
#define COMMANDBUFFER_H

#include "vulkan/vulkan.hpp"

namespace VKKit {
class Device;
class CommandPool;

// Wrapper over VkCommandBuffer
class CommandBuffer {
public:
    CommandBuffer() noexcept;
    CommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBufferLevel level);
    CommandBuffer(const Device& device, const CommandPool& pool, VkCommandBufferLevel level);
    ~CommandBuffer();

    CommandBuffer(const CommandBuffer& buf) = delete;
    CommandBuffer& operator=(const CommandBuffer& buf) = delete;
    CommandBuffer(CommandBuffer&& buf) noexcept;
    CommandBuffer& operator=(CommandBuffer&& buf) noexcept;

    void Begin(VkCommandBufferUsageFlags flags) const;
    void End() const;
    void Submit(VkQueue queue, bool device_wait) const;

    VkCommandBuffer GetBuffer() const noexcept { return buffer; }

private:
    VkDevice device;
    VkCommandPool pool;
    VkCommandBuffer buffer;
};
}

#endif