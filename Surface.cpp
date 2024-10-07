#include <vector>
#include "SDL_vulkan.h"
#include "Surface.h"
#include "VkResultString.h"
#include "Windowing.h"

namespace VKKit {
Surface::Surface() noexcept :
    instance{ nullptr }, surface{ nullptr }
{}

Surface::Surface(VkInstance instance, const Window& window) :
    instance{ instance }
{
    const auto result = SDL_Vulkan_CreateSurface(window.Get(), instance, &surface);
    if (result != SDL_TRUE) {
        char error[256];
        snprintf(error, sizeof(error), "Failed to create surface. Error: %s", SDL_GetError());
        throw std::runtime_error(error);
    }
}

Surface::~Surface()
{
    if (instance) vkDestroySurfaceKHR(instance, surface, nullptr);
}

Surface::Surface(Surface&& s) noexcept :
    instance{ s.instance }, surface{ s.surface }
{
    s.instance = nullptr;
    s.surface = nullptr;
}

Surface& Surface::operator=(Surface&& s) noexcept
{
    if (instance) vkDestroySurfaceKHR(instance, surface, nullptr);
    instance = s.instance;
    surface = s.surface;
    s.instance = nullptr;
    s.surface = nullptr;
    return *this;
}

VkSurfaceFormatKHR ChooseSurfaceFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface) noexcept
{
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats.data());

    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return formats[0];
}
}