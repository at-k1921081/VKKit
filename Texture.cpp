#include <expected>
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Texture.h"
#include "Buffer.h"
#include "VkResultString.h"
#include "CommandBuffer.h"
#include "Device.h"
#include "CommandPool.h"

namespace {
class ImageData {
public:
    ImageData() noexcept : pixels{ nullptr }, width{ 0 }, height{ 0 }, channels{ 0 } {}
    ImageData(std::string_view filepath, int req_comp)
    {
        pixels = stbi_load(filepath.data(), &width, &height, &channels, req_comp);
        if (!pixels) {
            char error[256];
            snprintf(error, sizeof(error), "Failed to create image from file %s. Error: %s", filepath.data(), stbi_failure_reason());
            throw std::runtime_error(error);
        }
    }

    ~ImageData()
    {
        stbi_image_free(pixels);
    }

    static std::expected<ImageData, std::string_view> Create(std::string_view filepath, int req_comp) noexcept
    {
        std::string_view error;
        ImageData img(filepath, req_comp, error);
        
        if (error.data()) return std::unexpected(error);
        else return img;
    }

    ImageData(const ImageData& img) = delete;
    ImageData& operator=(const ImageData& img) = delete;
    ImageData(ImageData&& img) noexcept : pixels{ img.pixels }, width{ img.width }, height{ img.height }, channels{ img.channels }
    {
        img.pixels = nullptr;
    }
    ImageData& operator=(ImageData&& img) noexcept
    {
        stbi_image_free(pixels);

        width = img.width;
        height = img.height;
        channels = img.channels;
        img.pixels = nullptr;

        return *this;
    }

    stbi_uc* GetPixels() const noexcept { return pixels; }
    int GetWidth() const noexcept { return width; }
    int GetHeight() const noexcept { return height; }
    int GetChannels() const noexcept { return channels; }

protected:
    ImageData(std::string_view filepath, int req_comp, std::string_view& error) noexcept :
        width{}, height{}, channels{}
    {
        pixels = stbi_load(filepath.data(), &width, &height, &channels, req_comp);
        if (!pixels) {
            error = stbi_failure_reason();
            return;
        }
    }

private:
    stbi_uc* pixels;
    int width, height, channels;
};
}

namespace VKKit {
static void CopyBufferToImage(const Device& device, const CommandPool& command_pool, const Buffer& buffer, const Image& image,
    uint32_t width, uint32_t height, VkQueue graphics_queue)
{
    const CommandBuffer command_buffer(device, command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    command_buffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    const VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,

        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },

        .imageOffset = { 0, 0, 0 },
        .imageExtent = { width, height, 1 }
    };

    vkCmdCopyBufferToImage(command_buffer.GetBuffer(), buffer.GetBuffer(), image.Get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    command_buffer.End();
    command_buffer.Submit(graphics_queue, true);
}

static void TransitionImageLayout(const Device& device, const CommandPool& command_pool, const Image& image, VkFormat format,
    VkImageLayout old_layout, VkImageLayout new_layout, VkQueue graphics_queue, uint32_t mip_levels)
{
    CommandBuffer command_buffer(device, command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    command_buffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = 0,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image.Get(),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = mip_levels,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkPipelineStageFlags source_stage, destination_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout ==
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
        throw std::invalid_argument("Unsupported layout transition");

    vkCmdPipelineBarrier(command_buffer.GetBuffer(), source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    command_buffer.End();
    command_buffer.Submit(graphics_queue, true);
}

static void GenerateMipmaps(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, VkQueue graphics_queue, const Image& image,
    VkFormat image_format, int32_t width, int32_t height, uint32_t mip_levels)
{
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(physical_device, image_format, &format_properties);
    if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        throw std::runtime_error("Texture image format doesn't support linear blitting");

    const CommandBuffer command_buffer(device, pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    command_buffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image.Get(),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    for (uint32_t i = 1; i < mip_levels; ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer.GetBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        const VkImageBlit blit = {
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i - 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .srcOffsets = {
                {0, 0, 0},
                {width, height, 1}
            },
            .dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .dstOffsets = {
                {0, 0, 0},
                {width > 1 ? width / 2 : 1, height > 1 ? height / 2 : 1, 1}
            }
        };

        vkCmdBlitImage(command_buffer.GetBuffer(), image.Get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image.Get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer.GetBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (width > 1) width /= 2;
        if (height > 1) height /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mip_levels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer.GetBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    command_buffer.End();
    command_buffer.Submit(graphics_queue, true);
}

static uint32_t CalculateMaxMipLevels(uint32_t width, uint32_t height)
{
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))));
}

Texture::Texture(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, VkFormat format, VkImageAspectFlags aspect,
    VkImageTiling tiling, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, uint32_t mip_levels,
    std::string_view filepath, bool alpha) :
    mipmap_levels{ mip_levels }
{
    const ImageData pixels(filepath, alpha ? STBI_rgb_alpha : STBI_rgb);
    width = pixels.GetWidth();
    height = pixels.GetHeight();
    channels = pixels.GetChannels();

    const VkDeviceSize size = pixels.GetWidth() * pixels.GetHeight() * sizeof(int);

    const Buffer staging_buffer(physical_device, device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    vkMapMemory(device.Get(), staging_buffer.GetMemory(), 0, size, 0, &data);
    memcpy(data, pixels.GetPixels(), size);
    vkUnmapMemory(device.Get(), staging_buffer.GetMemory());

    if (mipmap_levels == 0) mipmap_levels = CalculateMaxMipLevels(width, height);

    this->texture = Image(device, 0, VK_IMAGE_TYPE_2D, format, { static_cast<uint32_t>(pixels.GetWidth()), static_cast<uint32_t>(pixels.GetHeight()), 1 }, mipmap_levels,
        1, samples, tiling, usage, VK_SHARING_MODE_EXCLUSIVE, 0, nullptr, VK_IMAGE_LAYOUT_UNDEFINED);
        
    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device.Get(), texture.Get(), &mem_requirements);

    this->memory = DeviceMemory(device, mem_requirements.size, FindMemoryType(physical_device, mem_requirements.memoryTypeBits, properties));
    vkBindImageMemory(device.Get(), texture.Get(), memory.Get(), 0);

    TransitionImageLayout(device, pool, texture, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, device.GetGraphicsQueue(), mipmap_levels);
    CopyBufferToImage(device, pool, staging_buffer, texture, static_cast<uint32_t>(width),
        static_cast<uint32_t>(height), device.GetGraphicsQueue());
    GenerateMipmaps(physical_device, device, pool, device.GetGraphicsQueue(), texture, format, width, height, mipmap_levels);

    this->view = ImageView(device, 0, texture.Get(), VK_IMAGE_VIEW_TYPE_2D, format, VkComponentMapping{}, { aspect, 0, mipmap_levels, 0, 1 });
}

Texture::Texture(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, VkFormat format, uint32_t width,
    uint32_t height, VkImageAspectFlags aspect, VkImageTiling tiling, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
    uint32_t mip_levels) :
    width{ width }, height{ height }, mipmap_levels{ mip_levels }
{
    if (mipmap_levels == 0) mipmap_levels = CalculateMaxMipLevels(width, height);

    texture = Image(device, VkImageCreateFlags{}, VK_IMAGE_TYPE_2D, format, VkExtent3D{ width, height, 1 }, mipmap_levels, 1, samples, tiling, usage,
        VK_SHARING_MODE_EXCLUSIVE, 0, nullptr, VK_IMAGE_LAYOUT_UNDEFINED);

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device.Get(), texture.Get(), &mem_requirements);

    memory = DeviceMemory(device, mem_requirements.size, FindMemoryType(physical_device, mem_requirements.memoryTypeBits, properties));
    
    vkBindImageMemory(device.Get(), texture.Get(), memory.Get(), 0);

    view = ImageView(device, VkImageViewCreateFlags{}, texture.Get(), VK_IMAGE_VIEW_TYPE_2D, format, VkComponentMapping{}, { aspect, 0, mipmap_levels, 0, 1 });
}

Texture::Texture(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, VkFormat format, uint32_t width, uint32_t height,
    ut::rspan<const unsigned char> image_data, VkImageTiling tiling, VkSampleCountFlagBits samples, uint32_t mips)
{
    if (mips == 0) mips = CalculateMaxMipLevels(width, height);

    // const VkDeviceSize size = width * height * sizeof(uint32_t); // Each texel is sizeof(uint32_t) big
    const VkDeviceSize size = image_data.size_bytes();

    const Buffer staging = CreateStagingBuffer(physical_device, device.Get(), size);

    void* data;
    vkMapMemory(device.Get(), staging.GetMemory(), 0, size, 0, &data);
    memcpy(data, image_data.data(), size);
    vkUnmapMemory(device.Get(), staging.GetMemory());

    this->texture = Image(device, 0, VK_IMAGE_TYPE_2D, format, { width, height, 1 }, mips, 1, samples, tiling, VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT, VK_SHARING_MODE_EXCLUSIVE, 0, nullptr, VK_IMAGE_LAYOUT_UNDEFINED);

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device.Get(), texture.Get(), &mem_requirements);

    this->memory = DeviceMemory(device, mem_requirements.size, FindMemoryType(physical_device, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
    vkBindImageMemory(device.Get(), texture.Get(), memory.Get(), 0);

    TransitionImageLayout(device, pool, texture, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        device.GetGraphicsQueue(), mips);
    CopyBufferToImage(device, pool, staging, texture, width, height, device.GetGraphicsQueue());
    GenerateMipmaps(physical_device, device, pool, device.GetGraphicsQueue(), texture, format, width, height, mips);

    this->view = ImageView(device, 0, texture.Get(), VK_IMAGE_VIEW_TYPE_2D, format, {}, { VK_IMAGE_ASPECT_COLOR_BIT, 0, mips, 0, 1 });
}
}