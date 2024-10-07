#include "ImageObjects.h"
#include "VkResultString.h"
#include "Device.h"
#include "RenderPass.h"
#include "ImageObjects.h"

namespace VKKit {
DeviceMemory::DeviceMemory() noexcept :
    device{ nullptr }, memory{ nullptr }
{}

DeviceMemory::DeviceMemory(const Device& device, VkDeviceSize size, uint32_t memory_type_index) :
    device{ device.Get() }
{
    const VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = size,
        .memoryTypeIndex = memory_type_index
    };

    const auto result = vkAllocateMemory(device.Get(), &alloc_info, nullptr, &memory);
    if (result != VK_SUCCESS) ThrowError("Failed to allocate device memory.", result);
}

std::expected<DeviceMemory, std::string_view> DeviceMemory::Allocate(const Device& device, VkDeviceSize size, uint32_t memory_type_index) noexcept
{
    std::string_view error;
    DeviceMemory mem(device, size, memory_type_index, error);
    if (error.size() != 0) return std::unexpected(error);
    return mem;
}

DeviceMemory::DeviceMemory(const Device& device, VkDeviceSize size, uint32_t memory_type_index, std::string_view& error) noexcept
{
    const VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = size,
        .memoryTypeIndex = memory_type_index
    };

    const auto result = vkAllocateMemory(device.Get(), &alloc_info, nullptr, &memory);
    if (result != VK_SUCCESS) {
        error = VkResultString(result);
        this->device = nullptr;
    }
    else {
        this->device = device.Get();
    }
}

DeviceMemory::~DeviceMemory()
{
    if (device) vkFreeMemory(device, memory, nullptr);
}

DeviceMemory::DeviceMemory(DeviceMemory&& dm) noexcept :
    device{ dm.device }, memory{ dm.memory }
{
    dm.device = nullptr;
    dm.memory = nullptr;
}

DeviceMemory& DeviceMemory::operator=(DeviceMemory&& dm) noexcept
{
    if (device) vkFreeMemory(device, memory, nullptr);
    device = dm.device;
    memory = dm.memory;
    dm.device = nullptr;
    dm.memory = nullptr;
    return *this;
}

Image::Image() noexcept :
    device{ nullptr }, image{ nullptr }
{}

Image::Image(
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
) : device{ device.Get() }
{
    const VkImageCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = flags,
        .imageType = imageType,
        .format = format,
        .extent = extent,
        .mipLevels = mipLevels,
        .arrayLayers = arrayLayers,
        .samples = samples,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = sharingMode,
        .queueFamilyIndexCount = queueFamilyIndexCount,
        .pQueueFamilyIndices = pQueueFamilyIndices,
        .initialLayout = initialLayout
    };

    const auto result = vkCreateImage(device.Get(), &info, nullptr, &image);
    if (result != VK_SUCCESS) ThrowError("Failed to create image.", result);
}

std::expected<Image, std::string_view> Image::Create(
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
) noexcept
{
    std::string_view error;
    Image image(device, flags, imageType, format, extent, mipLevels, arrayLayers, samples, tiling, usage, sharingMode, queueFamilyIndexCount,
        pQueueFamilyIndices, initialLayout, error);
    if (error.data() != nullptr) return std::unexpected(error);
    return image;
}

Image::Image(
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
) noexcept
{
    const VkImageCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = flags,
        .imageType = imageType,
        .format = format,
        .extent = extent,
        .mipLevels = mipLevels,
        .arrayLayers = arrayLayers,
        .samples = samples,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = sharingMode,
        .queueFamilyIndexCount = queueFamilyIndexCount,
        .pQueueFamilyIndices = pQueueFamilyIndices,
        .initialLayout = initialLayout
    };

    const auto result = vkCreateImage(device.Get(), &info, nullptr, &image);
    if (result != VK_SUCCESS) {
        error = VkResultString(result);
        this->device = nullptr;
    }
    else {
        this->device = device.Get();
    }
}

Image::~Image()
{
    if (device) vkDestroyImage(device, image, nullptr);
}

Image::Image(Image&& i) noexcept :
    device{ i.device }, image{ i.image }
{
    i.device = nullptr;
    i.image = nullptr;
}

Image& Image::operator=(Image&& i) noexcept
{
    if (device) vkDestroyImage(device, image, nullptr);
    device = i.device;
    image= i.image;
    i.device = nullptr;
    i.image = nullptr;
    return *this;
}

ImageView::ImageView() noexcept :
    device{ nullptr }, view{ nullptr }
{}

ImageView::ImageView(
    const Device&              device,
    VkImageViewCreateFlags     flags,
    VkImage                    image,
    VkImageViewType            viewType,
    VkFormat                   format,
    VkComponentMapping         components,
    VkImageSubresourceRange    subresourceRange
) :
    device{ device.Get() }
{
    const VkImageViewCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags = flags,
        .image = image,
        .viewType = viewType,
        .format = format,
        .components = components,
        .subresourceRange = subresourceRange
    };

    const auto result = vkCreateImageView(device.Get(), &info, nullptr, &view);
    if (result != VK_SUCCESS) ThrowError("Failed to create image view.", result);
}

std::expected<ImageView, std::string_view> ImageView::Create(
    const Device&              device,
    VkImageViewCreateFlags     flags,
    VkImage                    image,
    VkImageViewType            viewType,
    VkFormat                   format,
    VkComponentMapping         components,
    VkImageSubresourceRange    subresourceRange
) noexcept
{
    std::string_view error;
    ImageView iv(device, flags, image, viewType, format, components, subresourceRange, error);
    if (error.data() != nullptr) return std::unexpected(error);
    return iv;
}

ImageView::ImageView(
    const Device&              device,
    VkImageViewCreateFlags     flags,
    VkImage                    image,
    VkImageViewType            viewType,
    VkFormat                   format,
    VkComponentMapping         components,
    VkImageSubresourceRange    subresourceRange,
    std::string_view&          error
) noexcept
{
    const VkImageViewCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags = flags,
        .viewType = viewType,
        .format = format,
        .components = components,
        .subresourceRange = subresourceRange
    };

    const auto result = vkCreateImageView(device.Get(), &info, nullptr, &view);
    if (result != VK_SUCCESS) {
        error = VkResultString(result);
        this->device = nullptr;
    }
    else {
        this->device = device.Get();
    }
}

ImageView::~ImageView()
{
    if (device) vkDestroyImageView(device, view, nullptr);
}

ImageView::ImageView(ImageView&& iv) noexcept :
    device{ iv.device }, view{ iv.view }
{
    iv.device = nullptr;
    iv.view = nullptr;
}

ImageView& ImageView::operator=(ImageView&& iv) noexcept
{
    if (device) vkDestroyImageView(device, view, nullptr);
    device = iv.device;
    view = iv.view;
    iv.device = nullptr;
    iv.view = nullptr;
    return *this;
}

Framebuffer::Framebuffer() noexcept :
    device{ nullptr }, framebuffer{ nullptr }
{}

Framebuffer::Framebuffer(
    const Device&               device,
    VkFramebufferCreateFlags    flags,
    const RenderPass&           renderPass,
    std::span<const VkImageView> attachments,
    uint32_t                    width,
    uint32_t                    height,
    uint32_t                    layers
) :
    device{ device.Get() }
{
    const VkFramebufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .flags = flags,
        .renderPass = renderPass.Get(),
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = width,
        .height = height,
        .layers = layers
    };

    const auto result = vkCreateFramebuffer(device.Get(), &info, nullptr, &framebuffer);
    if (result != VK_SUCCESS) ThrowError("Failed to create framebuffer.", result);
}

std::expected<Framebuffer, std::string_view> Framebuffer::Create(
    const Device&               device,
    VkFramebufferCreateFlags    flags,
    const RenderPass&           renderPass,
    std::span<const VkImageView> attachments,
    uint32_t                    width,
    uint32_t                    height,
    uint32_t                    layers
) noexcept
{
    std::string_view error;
    Framebuffer fb(device, flags, renderPass, attachments, width, height, layers, error);
    if (error.data() != nullptr) return std::unexpected(error);
    return fb;
}

Framebuffer::Framebuffer(
    const Device&               device,
    VkFramebufferCreateFlags    flags,
    const RenderPass&           renderPass,
    std::span<const VkImageView> attachments,
    uint32_t                    width,
    uint32_t                    height,
    uint32_t                    layers,
    std::string_view&           error
) noexcept
{
    const VkFramebufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .flags = flags,
        .renderPass = renderPass.Get(),
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = width,
        .height = height,
        .layers = layers
    };

    const auto result = vkCreateFramebuffer(device.Get(), &info, nullptr, &framebuffer);
    if (result != VK_SUCCESS) {
        error = VkResultString(result);
        this->device = nullptr;
    }
    else {
        this->device = device.Get();
    }
}

Framebuffer::~Framebuffer()
{
    if (device) vkDestroyFramebuffer(device, framebuffer, nullptr);
}

Framebuffer::Framebuffer(Framebuffer&& fb) noexcept :
    device{ fb.device }, framebuffer{ fb.framebuffer }
{
    fb.device = nullptr;
    fb.framebuffer = nullptr;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& fb) noexcept
{
    if (device) vkDestroyFramebuffer(device, framebuffer, nullptr);
    device = fb.device;
    framebuffer = fb.framebuffer;
    fb.device = nullptr;
    fb.framebuffer = nullptr;
    return *this;
}
}