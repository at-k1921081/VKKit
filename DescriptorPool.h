#ifndef VULKANDESCRIPTORPOOL_H
#define VULKANDESCRIPTORPOOL_H

#include <span>
#include "vulkan/vulkan.hpp"

namespace VKKit {
class Device;

// A Vulkan object that allocates memory for the different kinds of data that the pipeline will work with. Wrapper over VkDescriptorPool.
class DescriptorPool {
public:
    DescriptorPool() noexcept;

    /**
     * @brief Construct a descriptor pool
     * 
     * @param device The device which will work with the descriptor pool
     * @param flags The pool creation flags
     * @param pool_sizes The pools and their sizes
     * @param frames_in_flight The number of frames in flight used by the swapchain. Used to calculate how many descriptors the pool can allocate.
     * 
     * @throw std::runtime_error with error information on failure
     */
    DescriptorPool(VkDevice device, VkDescriptorPoolCreateFlags flags, std::span<const VkDescriptorPoolSize> pool_sizes, uint32_t frames_in_flight);

    /**
     * @brief Construct a descriptor pool
     * 
     * @param device The device which will work with the descriptor pool
     * @param flags The pool creation flags
     * @param pool_sizes The pools and their sizes
     * @param frames_in_flight The number of frames in flight used by the swapchain. Used to calculate how many descriptors the pool can allocate.
     * 
     * @throw std::runtime_error with error information on failure
     */
    DescriptorPool(const Device& device, VkDescriptorPoolCreateFlags flags, std::span<const VkDescriptorPoolSize> pool_sizes, uint32_t frames_in_flight);

    ~DescriptorPool();

    DescriptorPool(const DescriptorPool& dp) = delete;
    DescriptorPool& operator=(const DescriptorPool& dp) = delete;
    DescriptorPool(DescriptorPool&& dp) noexcept;
    DescriptorPool& operator=(DescriptorPool&& dp) noexcept;

    VkDescriptorPool Get() const noexcept { return pool; }

    void Reset() noexcept;

private:
    VkDevice device;
    VkDescriptorPool pool;
};
}

#endif