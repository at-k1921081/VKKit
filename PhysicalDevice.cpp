#include <vector>
#include <map>
#include <set>

#include "PhysicalDevice.h"
#include "Instance.h"
#include "Surface.h"
#include "Constants.h"

namespace {
struct StrEq {
    constexpr bool operator()(const char* str1, const char* str2) const
    {
        using Traits = std::char_traits<char>;

        return Traits::compare(str1, str2, std::min(Traits::length(str1), Traits::length(str2))) == 0;
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};
}

namespace VKKit {
static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

    details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

    details.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());

    return details;
}

static bool CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::set<const char*, StrEq> required_extensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

    for (const auto& extension : available_extensions)
        required_extensions.erase(extension.extensionName);

    return required_extensions.empty();
}

static unsigned RateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(device, &device_features);

    unsigned score = 0;

    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;
    score += device_properties.limits.maxImageDimension2D;

    if (!device_features.geometryShader) return 0;
    if (device_features.samplerAnisotropy != VK_TRUE) return 0;

    if (!CheckDeviceExtensionSupport(device)) return 0;
    const auto swapchain_support = QuerySwapChainSupport(device, surface);
    if (swapchain_support.formats.empty() || swapchain_support.present_modes.empty()) return 0;

    return score;
}

VkPhysicalDevice PickPhysicalDevice(const Instance& instance, const Surface& surface)
{
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance.Get(), &device_count, nullptr);

    if (device_count == 0)
        throw std::runtime_error("Failed to find GPUs with Vulkan support");

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance.Get(), &device_count, devices.data());

    std::multimap<unsigned, VkPhysicalDevice> candidates;

    for (const auto device : devices) {
        const auto score = RateDeviceSuitability(device, surface.Get());
        candidates.insert({score, device});
    }

    if (candidates.rbegin()->first == 0)
        throw std::runtime_error("Failed to find a suitable GPU");

    physical_device = candidates.rbegin()->second;
    return physical_device;
}

VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDevice physical_device) noexcept
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device, &properties);

    const auto counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
    if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
    if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
    if (counts & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
    if (counts & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
    if (counts & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;
    return VK_SAMPLE_COUNT_1_BIT;
}

std::expected<VkFormat, std::string_view> FindSupportedFormat(VkPhysicalDevice physical_device,
    std::span<const VkFormat> candidates, VkImageTiling tiling, VkFormatFeatureFlags features) noexcept
{
    for (const auto format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) return format;
    }

    return std::unexpected("Failed to find supported format");
}

std::expected<VkFormat, std::string_view> FindDepthFormat(VkPhysicalDevice physical_device) noexcept
{
    return FindSupportedFormat(physical_device,
        std::array<VkFormat, 3>{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkPhysicalDeviceProperties GetPhysicalDeviceProperties(VkPhysicalDevice physical_device) noexcept
{
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physical_device, &props);
    return props;
}
}