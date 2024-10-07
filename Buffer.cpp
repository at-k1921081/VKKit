#include "Buffer.h"
#include "VkResultString.h"
#include "rspan.h"
#include "CommandBuffer.h"
#include "Device.h"

namespace VKKit {
Buffer::Buffer() : device{ nullptr}, buffer{ nullptr }, memory{ nullptr }
{}

Buffer::Buffer(VkPhysicalDevice physical_device, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkSharingMode sharing_mode) : device{ device }
{
    const VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = sharing_mode
    };

    const auto buf_result = vkCreateBuffer(device, &buffer_info, nullptr, &buffer);
    if (buf_result != VK_SUCCESS) ThrowError("Failed to create buffer.", buf_result);

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

    const VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_requirements.size,
        .memoryTypeIndex = FindMemoryType(physical_device, mem_requirements.memoryTypeBits, properties)
    };

    const auto alloc_result = vkAllocateMemory(device, &alloc_info, nullptr, &memory);
    if (alloc_result != VK_SUCCESS) {
        vkDestroyBuffer(device, buffer, nullptr);
        ThrowError("Failed to allocate buffer memory.", alloc_result);
    }

    const auto bind_result = vkBindBufferMemory(device, buffer, memory, 0);
    if (bind_result != VK_SUCCESS) {
        vkFreeMemory(device, memory, nullptr);
        vkDestroyBuffer(device, buffer, nullptr);
        ThrowError("Failed to bind memory.", bind_result);
    }
}

Buffer::Buffer(VkPhysicalDevice physical_device, const Device& device, VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkSharingMode sharing_mode) :
    Buffer(physical_device, device.Get(), size, usage, properties, sharing_mode)
{}

Buffer Buffer::CreateVertexBuffer(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, ut::rspan<const float> vertices,
    VkSharingMode sharing_mode)
{
    const VkDeviceSize size = vertices.size_bytes();

    const Buffer staging(physical_device, device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sharing_mode);

    void* data;
    vkMapMemory(device.Get(), staging.GetMemory(), 0, size, 0, &data);
    memcpy(data, vertices.data(), size);
    vkUnmapMemory(device.Get(), staging.GetMemory());

    Buffer vertex(physical_device, device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sharing_mode);

    CopyBuffer(device, pool, vertex, staging, size);

    return vertex;
}

Buffer Buffer::CreateIndexBuffer(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, ut::rspan<const uint32_t> indices,
    VkSharingMode sharing_mode)
{
    const VkDeviceSize size = indices.size_bytes();

    const Buffer staging(physical_device, device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sharing_mode);

    void* data;
    vkMapMemory(device.Get(), staging.GetMemory(), 0, size, 0, &data);
    memcpy(data, indices.data(), size);
    vkUnmapMemory(device.Get(), staging.GetMemory());

    Buffer index(physical_device, device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sharing_mode);

    CopyBuffer(device, pool, index, staging, size);

    return index;
}

Buffer::~Buffer()
{
    if (device) {
        vkFreeMemory(device, memory, nullptr);
        vkDestroyBuffer(device, buffer, nullptr);
    }
}

Buffer::Buffer(Buffer&& b) noexcept :
    device{ b.device }, buffer{ b.buffer }, memory{ b.memory }
{
    b.device = nullptr;
    b.buffer = nullptr;
    b.memory = nullptr;
}

Buffer& Buffer::operator=(Buffer&& b) noexcept
{
    if (device) {
        vkFreeMemory(device, memory, nullptr);
        vkDestroyBuffer(device, buffer, nullptr);
    }

    device = b.device;
    buffer = b.buffer;
    memory = b.memory;

    b.device = nullptr;
    b.buffer = nullptr;
    b.memory = nullptr;

    return *this;
}

void Buffer::WriteData(const Device& device, const CommandPool& command_pool, const Buffer& staging, const void* input,
    VkDeviceSize size) const
{
    void* data;
    vkMapMemory(device.Get(), staging.GetMemory(), 0, size, 0, &data);
    memcpy(data, input, size); // TO DO: Fix this, causes access violation
    vkUnmapMemory(device.Get(), staging.GetMemory());

    CopyBuffer(device, command_pool, *this, staging, size);
}

uint32_t FindMemoryType(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i) {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("Failed to find suitable memory type");
}

Buffer CreateStagingBuffer(VkPhysicalDevice physical_device, VkDevice device, VkDeviceSize size)
{
    return Buffer(physical_device, device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void CopyBuffer(const Device& device, const CommandPool& command_pool, const Buffer& dst, const Buffer& src, VkDeviceSize size)
{
    const CommandBuffer command_buffer(device, command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    command_buffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    const VkBufferCopy copy_region = {
        .size = size
    };
    vkCmdCopyBuffer(command_buffer.GetBuffer(), src.GetBuffer(), dst.GetBuffer(), 1, &copy_region);

    command_buffer.End();
    command_buffer.Submit(device.GetGraphicsQueue(), true);
}
}