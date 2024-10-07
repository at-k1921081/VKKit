#ifndef BUFFER_H
#define BUFFER_H

#include "vulkan/vulkan.h"
#include "rspan.h"
#include "RenderData.h"

namespace VKKit {
class Device;
class CommandPool;

class Buffer {
public:
    Buffer();
    Buffer(VkPhysicalDevice physical_device, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties, VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE);
    Buffer(VkPhysicalDevice physical_device, const Device& device, VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties, VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE);
    ~Buffer();

    static Buffer CreateVertexBuffer(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, ut::rspan<const float> vertices,
        VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE);

    static Buffer CreateIndexBuffer(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, ut::rspan<const uint32_t> indices,
        VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE);

    Buffer(const Buffer& b) = delete;
    Buffer& operator=(const Buffer& b) = delete;
    Buffer(Buffer&& b) noexcept;
    Buffer& operator=(Buffer&& b) noexcept;

    /**
     * @brief Write data to this buffer. This function should only be used for device local buffers (not staging,
     *        the ones used when drawing).
     * @param device The logical device used
     * @param command_pool The command pool used
     * @param staging The staging buffer where the initial data will be written before being copied to this device local buffer
     * @param input The data to copy
     * @param size The size of the data
     */
    void WriteData(const Device& device, const CommandPool& command_pool, const Buffer& staging, const void* input,
        VkDeviceSize size) const;

    VkBuffer GetBuffer() const { return buffer; }
    VkDeviceMemory GetMemory() const { return memory; }

private:
    VkDevice device;
    VkBuffer buffer;
    VkDeviceMemory memory;
};

uint32_t FindMemoryType(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties);
Buffer CreateStagingBuffer(VkPhysicalDevice physical_device, VkDevice device, VkDeviceSize size);

void CopyBuffer(const Device& device, const CommandPool& command_pool, const Buffer& dst, const Buffer& src, VkDeviceSize size);
}

#endif