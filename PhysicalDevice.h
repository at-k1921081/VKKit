#ifndef PHYSICALDEVICE_H
#define PHYSICALDEVICE_H

#include <expected>
#include <string_view>
#include <span>
#include "vulkan/vulkan.h"

namespace VKKit {
class Instance;
class Surface;

VkPhysicalDevice PickPhysicalDevice(const Instance& instance, const Surface& surface);

VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDevice physical_device) noexcept;

std::expected<VkFormat, std::string_view> FindSupportedFormat(VkPhysicalDevice physical_device,
    std::span<const VkFormat> candidates, VkImageTiling tiling, VkFormatFeatureFlags features) noexcept;

std::expected<VkFormat, std::string_view> FindDepthFormat(VkPhysicalDevice physical_device) noexcept;

VkPhysicalDeviceProperties GetPhysicalDeviceProperties(VkPhysicalDevice physical_device) noexcept;
}

#endif