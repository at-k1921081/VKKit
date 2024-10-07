#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include <vector>
#include "vulkan/vulkan.hpp"
#include "ImageObjects.h"
#include "Texture.h"

namespace VKKit {
class Device;
class CommandPool;
class Surface;
class Window;
class RenderPass;

class Swapchain {
public:
    Swapchain();

    Swapchain(VkPhysicalDevice physical_device, const Device& device, const CommandPool& command_pool, const Surface& surface, const Window& window,
        const RenderPass& render_pass, VkSurfaceFormatKHR surface_format, VkPresentModeKHR present_mode, VkImageUsageFlags image_usage,
        VkCompositeAlphaFlagBitsKHR composite_alpha, VkBool32 clipped, VkSampleCountFlagBits samples, VkSwapchainKHR old_swapchain);

    VkSwapchainKHR Get() const noexcept { return swapchain.Get(); }
    VkFormat GetFormat() const noexcept { return format; }
    uint32_t GetWidth() const noexcept { return extent.width; }
    uint32_t GetHeight() const noexcept { return extent.height; }
    VkExtent2D GetExtent() const noexcept { return extent; }
    const std::vector<Framebuffer>& GetFramebuffers() { return framebuffers; }

private:
    class SwapchainWrapper {
    public:
        SwapchainWrapper() noexcept;

        SwapchainWrapper(
            VkDevice                         device,
            VkSwapchainCreateFlagsKHR        flags,
            VkSurfaceKHR                     surface,
            uint32_t                         minImageCount,
            VkFormat                         imageFormat,
            VkColorSpaceKHR                  imageColorSpace,
            VkExtent2D                       imageExtent,
            uint32_t                         imageArrayLayers,
            VkImageUsageFlags                imageUsage,
            VkSharingMode                    imageSharingMode,
            uint32_t                         queueFamilyIndexCount,
            const uint32_t*                  pQueueFamilyIndices,
            VkSurfaceTransformFlagBitsKHR    preTransform,
            VkCompositeAlphaFlagBitsKHR      compositeAlpha,
            VkPresentModeKHR                 presentMode,
            VkBool32                         clipped,
            VkSwapchainKHR                   oldSwapchain
        );

        ~SwapchainWrapper();

        SwapchainWrapper(const SwapchainWrapper&) = delete;
        SwapchainWrapper& operator=(const SwapchainWrapper&) = delete;
        SwapchainWrapper(SwapchainWrapper&& sw) noexcept;
        SwapchainWrapper& operator=(SwapchainWrapper&& sw) noexcept;

        VkSwapchainKHR Get() const noexcept { return swapchain; }

    private:
        VkDevice device;
        VkSwapchainKHR swapchain;
    };

    SwapchainWrapper swapchain;
    VkFormat format;
    VkExtent2D extent;
    uint32_t min_images;
    std::vector<VkImage> images;
    std::vector<ImageView> views;
    Texture color_image, depth_image;
    std::vector<Framebuffer> framebuffers;
};
}

#endif