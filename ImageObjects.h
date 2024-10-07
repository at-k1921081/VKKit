#ifndef IMAGEOBJECTS_H
#define IMAGEOBJECTS_H

#include <expected>
#include <string_view>
#include <span>
#include "vulkan/vulkan.hpp"

namespace VKKit {
class Device;
class RenderPass;

// Vulkan device memory. Wrapper over VkDeviceMemory.
class DeviceMemory {
public:
    DeviceMemory() noexcept;

    DeviceMemory(const Device& device, VkDeviceSize size, uint32_t memory_type_index);
    static std::expected<DeviceMemory, std::string_view> Allocate(const Device& device, VkDeviceSize size, uint32_t memory_type_index) noexcept;

    ~DeviceMemory();

    DeviceMemory(const DeviceMemory&) = delete;
    DeviceMemory& operator=(const DeviceMemory&) = delete;
    DeviceMemory(DeviceMemory&& dm) noexcept;
    DeviceMemory& operator=(DeviceMemory&& dm) noexcept;

    VkDeviceMemory Get() const noexcept { return memory; }

protected:
    DeviceMemory(const Device& device, VkDeviceSize size, uint32_t memory_type_index, std::string_view& error) noexcept;

private:
    VkDevice device;
    VkDeviceMemory memory;
};

// Vulkan image. Wrapper over VkImage.
class Image {
public:
    Image() noexcept;

    Image(
        const Device&            device,
        VkImageCreateFlags       flags,
        VkImageType              imageType,
        VkFormat                 format,
        VkExtent3D               extent,
        uint32_t                 mipLevels,
        uint32_t                 arrayLayers,
        VkSampleCountFlagBits    samples,
        VkImageTiling            tiling,
        VkImageUsageFlags        usage,
        VkSharingMode            sharingMode,
        uint32_t                 queueFamilyIndexCount,
        const uint32_t*          pQueueFamilyIndices,
        VkImageLayout            initialLayout
    );
    
    static std::expected<Image, std::string_view> Create(
        const Device&            device,
        VkImageCreateFlags       flags,
        VkImageType              imageType,
        VkFormat                 format,
        VkExtent3D               extent,
        uint32_t                 mipLevels,
        uint32_t                 arrayLayers,
        VkSampleCountFlagBits    samples,
        VkImageTiling            tiling,
        VkImageUsageFlags        usage,
        VkSharingMode            sharingMode,
        uint32_t                 queueFamilyIndexCount,
        const uint32_t*          pQueueFamilyIndices,
        VkImageLayout            initialLayout
    ) noexcept;

    ~Image();

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&& i) noexcept;
    Image& operator=(Image&& i) noexcept;

    VkImage Get() const noexcept { return image; }

protected:
    Image(
        const Device&            device,
        VkImageCreateFlags       flags,
        VkImageType              imageType,
        VkFormat                 format,
        VkExtent3D               extent,
        uint32_t                 mipLevels,
        uint32_t                 arrayLayers,
        VkSampleCountFlagBits    samples,
        VkImageTiling            tiling,
        VkImageUsageFlags        usage,
        VkSharingMode            sharingMode,
        uint32_t                 queueFamilyIndexCount,
        const uint32_t*          pQueueFamilyIndices,
        VkImageLayout            initialLayout,
        std::string_view&        error
    ) noexcept;

private:
    VkDevice device;
    VkImage image;
};

// Vulkan image view. Wrapper over VkImageView.
class ImageView {
public:
    ImageView() noexcept;

    ImageView(
        const Device&              device,
        VkImageViewCreateFlags     flags,
        VkImage                    image,
        VkImageViewType            viewType,
        VkFormat                   format,
        VkComponentMapping         components,
        VkImageSubresourceRange    subresourceRange
    );

    static std::expected<ImageView, std::string_view> Create(
        const Device&              device,
        VkImageViewCreateFlags     flags,
        VkImage                    image,
        VkImageViewType            viewType,
        VkFormat                   format,
        VkComponentMapping         components,
        VkImageSubresourceRange    subresourceRange
    ) noexcept;

    ~ImageView();

    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;
    ImageView(ImageView&& iv) noexcept;
    ImageView& operator=(ImageView&& iv) noexcept;

    VkImageView Get() const noexcept { return view; }

protected:
    ImageView(
        const Device&              device,
        VkImageViewCreateFlags     flags,
        VkImage                    image,
        VkImageViewType            viewType,
        VkFormat                   format,
        VkComponentMapping         components,
        VkImageSubresourceRange    subresourceRange,
        std::string_view&          error
    ) noexcept;

private:
    VkDevice device;
    VkImageView view;
};

// Vulkan framebuffer. Wrapper over VkFramebuffer.
class Framebuffer {
public:
    Framebuffer() noexcept;

    Framebuffer(
        const Device&               device,
        VkFramebufferCreateFlags    flags,
        const RenderPass&           renderPass,
        std::span<const VkImageView>      attachments,
        uint32_t                    width,
        uint32_t                    height,
        uint32_t                    layers
    );

    static std::expected<Framebuffer, std::string_view> Create(
        const Device&               device,
        VkFramebufferCreateFlags    flags,
        const RenderPass&           renderPass,
        std::span<const VkImageView>      attachments,
        uint32_t                    width,
        uint32_t                    height,
        uint32_t                    layers
    ) noexcept;

    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&& fb) noexcept;
    Framebuffer& operator=(Framebuffer&& fb) noexcept;

    VkFramebuffer Get() const noexcept { return framebuffer; }

protected:
    Framebuffer(
        const Device&               device,
        VkFramebufferCreateFlags    flags,
        const RenderPass&           renderPass,
        std::span<const VkImageView> attachments,
        uint32_t                    width,
        uint32_t                    height,
        uint32_t                    layers,
        std::string_view&           error
    ) noexcept;

private:
    VkDevice device;
    VkFramebuffer framebuffer;
};
}

#endif