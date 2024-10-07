#ifndef VULKANDESCRIPTORSETLAYOUT_H
#define VULKANDESCRIPTORSETLAYOUT_H

#include <span>
#include "vulkan/vulkan.hpp"

namespace VKKit {
class Device;

// An object that describes the type of data that the pipeline shaders will work with (uniform buffers, samplers, etc.). Wrapper around VkDescriptorSetLayout.
class DescriptorSetLayout {
public:
    DescriptorSetLayout() noexcept;

    /**
     * @brief Construct a descriptor set layout
     * 
     * @param device The device which will work with the layout
     * @param bindings The bindings to the layout descriptions. Describes which stage of the pipeline will work with what data
     * 
     * @throw std::runtime_error with error information on failure
     */
    DescriptorSetLayout(VkDevice device, std::span<const VkDescriptorSetLayoutBinding> bindings);

    /**
     * @brief Construct a descriptor set layout
     * 
     * @param device The device which will work with the layout
     * @param bindings The bindings to the layout descriptions. Describes which stage of the pipeline will work with what data
     * 
     * @throw std::runtime_error with error information on failure
     */
    DescriptorSetLayout(const Device& device, std::span<const VkDescriptorSetLayoutBinding> bindings);
    
    ~DescriptorSetLayout();

    DescriptorSetLayout(const DescriptorSetLayout& dsl) = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout& dsl) = delete;
    DescriptorSetLayout(DescriptorSetLayout&& dsl) noexcept;
    DescriptorSetLayout& operator=(DescriptorSetLayout&& dsl) noexcept;

    VkDescriptorSetLayout Get() const noexcept { return layout; }

private:
    VkDevice device;
    VkDescriptorSetLayout layout;
};
}

#endif