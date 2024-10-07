#ifndef VULKANDEVICE_H
#define VULKANDEVICE_H

#include <span>
#include "vulkan/vulkan.hpp"

namespace VKKit {
// Vulkan logical device. Wrapper around VkDevice.
class Device {
public:
    Device() noexcept;

    /**
     * @brief Construct a logical device
     * 
     * @param physical_device The physical device which the logical device will abstract
     * @param features The features the device needs to have
     * @param surface The surface which the device will work with
     * @param extensions The necessary extensions
     * 
     * @throw std::runtime_error with error information on failure
     */
    Device(VkPhysicalDevice physical_device, const VkPhysicalDeviceFeatures& features, VkSurfaceKHR surface,
        std::span<const char* const> extensions);

#ifndef NDEBUG
    /**
     * @brief Construct a logical device with debugging features
     * 
     * @param physical_device The physical device which the logical device will abstract
     * @param features The features the device needs to have
     * @param surface The surface which the device will work with
     * @param extensions The necessary extensions
     * @param validation_layers The validation layers that the device will work with
     * 
     * @throw std::runtime_error with error information on failure
     */
    Device(VkPhysicalDevice physical_device, const VkPhysicalDeviceFeatures& features, VkSurfaceKHR surface,
        std::span<const char* const> extensions, std::span<const char* const> validation_layers);
#endif

    ~Device();

    Device(const Device& d) = delete;
    Device& operator=(const Device& d) = delete;
    Device(Device&& d) noexcept;
    Device& operator=(Device&& d) noexcept;

    VkDevice Get() const noexcept { return device; }
    uint32_t GetGraphicsQueueIndex() const noexcept { return graphics_queue_index; }
    uint32_t GetPresentQueueIndex() const noexcept { return present_queue_index; }
    VkQueue GetGraphicsQueue() const noexcept { return graphics_queue; }
    VkQueue GetPresentQueue() const noexcept { return present_queue; }

    void Wait() const noexcept;

private:
    VkDevice device;
    uint32_t graphics_queue_index, present_queue_index;
    VkQueue graphics_queue, present_queue;
};
}

#endif