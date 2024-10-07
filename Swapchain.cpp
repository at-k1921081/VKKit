#include "SDL_vulkan.h"
#include "Swapchain.h"
#include "VkResultString.h"
#include "Device.h"
#include "CommandPool.h"
#include "Surface.h"
#include "Windowing.h"
#include "RenderPass.h"

namespace {
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkPresentModeKHR> present_modes;
};
}

namespace VKKit {
static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, const Surface& surface)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface.Get(), &details.capabilities);

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface.Get(), &present_mode_count, nullptr);

    details.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface.Get(), &present_mode_count, details.present_modes.data());

    return details;
}

static VkExtent2D ChooseSwapExtent(const Window& window, const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        int w, h;
        SDL_Vulkan_GetDrawableSize(window.Get(), &w, &h);

        return VkExtent2D {
            std::clamp(static_cast<uint32_t>(w), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp(static_cast<uint32_t>(h), capabilities.minImageExtent.width, capabilities.maxImageExtent.width)
        };
    }
}

static std::expected<VkFormat, std::string_view> FindSupportedFormat(VkPhysicalDevice physical_device,
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

static std::expected<VkFormat, std::string_view> FindDepthFormat(VkPhysicalDevice physical_device) noexcept
{
    return FindSupportedFormat(physical_device,
        std::array<VkFormat, 3>{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

Swapchain::SwapchainWrapper::SwapchainWrapper() noexcept :
    device{ nullptr }, swapchain{ nullptr }
{}

Swapchain::SwapchainWrapper::SwapchainWrapper(
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
) :
    device{ device }
{
    const VkSwapchainCreateInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .flags = flags,
        .surface = surface,
        .minImageCount = minImageCount,
        .imageFormat = imageFormat,
        .imageColorSpace = imageColorSpace,
        .imageExtent = imageExtent,
        .imageArrayLayers = imageArrayLayers,
        .imageUsage = imageUsage,
        .imageSharingMode = imageSharingMode,
        .queueFamilyIndexCount = queueFamilyIndexCount,
        .pQueueFamilyIndices = pQueueFamilyIndices,
        .preTransform = preTransform,
        .compositeAlpha = compositeAlpha,
        .presentMode = presentMode,
        .clipped = clipped,
        .oldSwapchain = oldSwapchain
    };

    const auto result = vkCreateSwapchainKHR(device, &info, nullptr, &swapchain);
    if (result != VK_SUCCESS) ThrowError("Failed to create swapchain.", result);
}

Swapchain::SwapchainWrapper::~SwapchainWrapper()
{
    if (device) vkDestroySwapchainKHR(device, swapchain, nullptr);
}

Swapchain::SwapchainWrapper::SwapchainWrapper(SwapchainWrapper&& sw) noexcept :
    device{ sw.device }, swapchain{ sw.swapchain }
{
    sw.device = nullptr;
    sw.swapchain = nullptr;
}

Swapchain::SwapchainWrapper& Swapchain::SwapchainWrapper::operator=(SwapchainWrapper&& sw) noexcept
{
    if (device) vkDestroySwapchainKHR(device, swapchain, nullptr);
    device = sw.device;
    swapchain = sw.swapchain;
    sw.device = nullptr;
    sw.swapchain = nullptr;
    return *this;
}

Swapchain::Swapchain() :
    swapchain{}, format{ VkFormat{} }, min_images{ 0 }, images{}, views{}, framebuffers{}
{}

Swapchain::Swapchain(VkPhysicalDevice physical_device, const Device& device, const CommandPool& command_pool, const Surface& surface, const Window& window,
    const RenderPass& render_pass, VkSurfaceFormatKHR surface_format, VkPresentModeKHR present_mode, VkImageUsageFlags image_usage,
    VkCompositeAlphaFlagBitsKHR composite_alpha, VkBool32 clipped, VkSampleCountFlagBits samples, VkSwapchainKHR old_swapchain)
{
    const auto swapchain_support = QuerySwapChainSupport(physical_device, surface);

    std::array<uint32_t, 2> queue_family_indices = { device.GetGraphicsQueueIndex(), device.GetPresentQueueIndex() };
    const bool same_family = queue_family_indices[0] == queue_family_indices[1];
    extent = ChooseSwapExtent(window, swapchain_support.capabilities);
    min_images = swapchain_support.capabilities.maxImageCount > 0 && swapchain_support.capabilities.minImageCount >
        swapchain_support.capabilities.maxImageCount - 1 ? swapchain_support.capabilities.maxImageCount :
        swapchain_support.capabilities.minImageCount + 1;

    swapchain = SwapchainWrapper(device.Get(), {}, surface.Get(), min_images, surface_format.format, surface_format.colorSpace,
        extent, 1, image_usage, same_family ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
        same_family ? 0 : 2, same_family ? nullptr : queue_family_indices.data(), swapchain_support.capabilities.currentTransform,
        composite_alpha, present_mode, clipped, old_swapchain);

    format = surface_format.format;

    images.resize(min_images);
    const auto get_images = vkGetSwapchainImagesKHR(device.Get(), swapchain.Get(), &min_images, images.data());
    if (get_images != VK_SUCCESS) ThrowError("Failed to get swapchain images.", get_images);

    views.resize(min_images);
    for (size_t i = 0; i < images.size(); ++i) views[i] = ImageView(device, {}, images[i], VK_IMAGE_VIEW_TYPE_2D, format, {},
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    color_image = Texture(physical_device, device, command_pool, format, extent.width, extent.height,
        VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_TILING_OPTIMAL, samples, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1);

    const auto depth_format_result = FindDepthFormat(physical_device);
    if (!depth_format_result.has_value()) ThrowError("Failed to find depth format.", depth_format_result.error());
    depth_image = Texture(physical_device, device, command_pool, *depth_format_result, extent.width, extent.height,
        VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_TILING_OPTIMAL, samples, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1);

    framebuffers.resize(min_images);
    for (size_t i = 0; i < views.size(); ++i) {
        const std::array<VkImageView, 3> attachments {
            color_image.GetView(),
            views[i].Get(),
            depth_image.GetView()
        };

        framebuffers[i] = Framebuffer(device, {}, render_pass, attachments, extent.width, extent.height, 1);
    }
}
}