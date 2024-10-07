#ifndef VULKANSURFACE_H
#define VULKANSURFACE_H

#include "vulkan/vulkan.h"

namespace VKKit {
class Window;

// A window surface to draw on. Wrapper over VkSurfaceKHR.
class Surface {
public:
    Surface() noexcept;

    /**
     * @brief Construct a window surface for a given window
     * 
     * @param instance The instance which will handle the surface
     * @param window The window where the surface will be created
     * 
     * @throw std::runtime_error with error information on failure
     */
    Surface(VkInstance instance, const Window& window);
    
    ~Surface();

    Surface(const Surface& s) = delete;
    Surface& operator=(const Surface& s) = delete;
    Surface(Surface&& s) noexcept;
    Surface& operator=(Surface&& s) noexcept;

    VkSurfaceKHR Get() const noexcept { return surface; }

private:
    VkInstance instance;
    VkSurfaceKHR surface;
};

VkSurfaceFormatKHR ChooseSurfaceFormat(VkPhysicalDevice physical_device, VkSurfaceKHR surface) noexcept;
}

#endif