#include <expected>
#include <vector>
#include "Device.h"
#include "VkResultString.h"

namespace {
struct QueueFamilies {
    uint32_t graphics, present;
};
}

namespace VKKit {
static std::expected<QueueFamilies, std::string_view> FindQueueFamilies(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    uint32_t nfamilies;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &nfamilies, nullptr);

    std::vector<VkQueueFamilyProperties> properties(nfamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &nfamilies, properties.data());

    QueueFamilies families{};
    bool found_graphics = false;
    bool found_present = false;

    for (uint32_t i = 0; i < properties.size(); ++i) {
        if (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            families.graphics = i;
            found_graphics = true;
        }

        VkBool32 present_support;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
        if (present_support) {
            families.present = i;
            found_present = true;
        }

        if (found_graphics && found_present) {
            if (families.graphics == families.present) break; // Best to have graphics and presentation in one queue
        }
    }

    if (!found_graphics) return std::unexpected("Failed to find graphics queue");
    if (!found_present) return std::unexpected("Failed to find present queue");
    return families;
}

Device::Device() noexcept :
    device{ nullptr }, graphics_queue_index{ 0 }, present_queue_index{ 0 }, graphics_queue{ nullptr }, present_queue{ nullptr }
{}

Device::Device(VkPhysicalDevice physical_device, const VkPhysicalDeviceFeatures& features, VkSurfaceKHR surface,
    std::span<const char* const> extensions)
{
    const auto indices = FindQueueFamilies(physical_device, surface);
    if (!indices.has_value())
        ThrowError("Failed to find queue family.", indices.error());
    graphics_queue_index = indices->graphics;
    present_queue_index = indices->present;

    const uint32_t families = graphics_queue_index == present_queue_index ? 1 : 2;
    const float priority = 1.0f;
    std::array<VkDeviceQueueCreateInfo, 2> queue_create_infos;

    for (uint32_t i = 0; i < families; ++i) {
        queue_create_infos[i] = VkDeviceQueueCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = i == 0 ? graphics_queue_index : present_queue_index,
            .queueCount = 1,
            .pQueuePriorities = &priority
        };
    }

    const VkDeviceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = families,
        .pQueueCreateInfos = queue_create_infos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = &features
    };

    const auto result = vkCreateDevice(physical_device, &info, nullptr, &device);
    if (result != VK_SUCCESS) ThrowError("Failed to create logical device.", result);

    vkGetDeviceQueue(device, graphics_queue_index, 0, &graphics_queue);
    vkGetDeviceQueue(device, present_queue_index, 0, &present_queue);
}

#ifndef NDEBUG
Device::Device(VkPhysicalDevice physical_device, const VkPhysicalDeviceFeatures& features, VkSurfaceKHR surface,
    std::span<const char* const> extensions, std::span<const char* const> validation_layers)
{
    const auto indices = FindQueueFamilies(physical_device, surface);
    if (!indices.has_value())
        ThrowError("Failed to find queue family.", indices.error());
    graphics_queue_index = indices->graphics;
    present_queue_index = indices->present;

    const uint32_t families = graphics_queue_index == present_queue_index ? 1 : 2;
    const float priority = 1.0f;
    std::array<VkDeviceQueueCreateInfo, 2> queue_create_infos;

    for (uint32_t i = 0; i < families; ++i) {
        queue_create_infos[i] = VkDeviceQueueCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = i == 0 ? graphics_queue_index : present_queue_index,
            .queueCount = 1,
            .pQueuePriorities = &priority
        };
    }

    const VkDeviceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = families,
        .pQueueCreateInfos = queue_create_infos.data(),
        .enabledLayerCount = static_cast<uint32_t>(validation_layers.size()),
        .ppEnabledLayerNames = validation_layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = &features
    };

    const auto result = vkCreateDevice(physical_device, &info, nullptr, &device);
    if (result != VK_SUCCESS) ThrowError("Failed to create logical device.", result);

    vkGetDeviceQueue(device, graphics_queue_index, 0, &graphics_queue);
    vkGetDeviceQueue(device, present_queue_index, 0, &present_queue);
}
#endif

Device::~Device()
{
    vkDestroyDevice(device, nullptr);
}

Device::Device(Device&& d) noexcept :
    device{ d.device }, graphics_queue_index{ d.graphics_queue_index }, present_queue_index{ d.present_queue_index},
    graphics_queue{ d.graphics_queue }, present_queue{ d.present_queue }
{
    d.device = nullptr;
    d.graphics_queue = nullptr;
    d.present_queue = nullptr;
}

Device& Device::operator=(Device&& d) noexcept
{
    vkDestroyDevice(device, nullptr);
    device = d.device;
    graphics_queue_index = d.graphics_queue_index;
    present_queue_index = d.present_queue_index;
    graphics_queue = d.graphics_queue;
    present_queue = d.present_queue;
    d.device = nullptr;
    d.graphics_queue = nullptr;
    d.present_queue = nullptr;
    return *this;
}

void Device::Wait() const noexcept
{
    const auto wait = vkDeviceWaitIdle(device);
    (void)wait;
    assert(wait == VK_SUCCESS);
}
}