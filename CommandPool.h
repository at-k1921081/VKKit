#ifndef VULKANCOMMANDPOOL_H
#define VULKANCOMMANDPOOL_H

#include "vulkan/vulkan.hpp"

namespace VKKit {
class Device;

// A Vulkan object that allocates memory for command buffers. Wrapper over VkCommandPool.
class CommandPool {
public:
    CommandPool() noexcept;
    
    /**
     * @brief Construct a command pool
     * 
     * @param device The device which will use the command pool
     * @param flags Creation flags
     * @param queue_family The queue family the pool's command buffers will work with
     * 
     * @throw std::runtime_error with error information on failure
     */
    CommandPool(VkDevice device, VkCommandPoolCreateFlags flags, uint32_t queue_family);

    /**
     * @brief Construct a command pool
     * 
     * @param device The device which will use the command pool
     * @param flags Creation flags
     * @param queue_family The queue family the pool's command buffers will work with
     * 
     * @throw std::runtime_error with error information on failure
     */
    CommandPool(const Device& device, VkCommandPoolCreateFlags flags, uint32_t queue_family);

    ~CommandPool();

    CommandPool(const CommandPool& cp) = delete;
    CommandPool& operator=(const CommandPool& cp) = delete;
    CommandPool(CommandPool&& cp) noexcept;
    CommandPool& operator=(CommandPool&& cp) noexcept;

    VkCommandPool Get() const noexcept { return pool; }

private:
    VkDevice device;
    VkCommandPool pool;
};
}

#endif