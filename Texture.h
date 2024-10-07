#ifndef TEXTURE_H
#define TEXTURE_H

#include <expected>
#include "vulkan/vulkan.hpp"
#include "ImageObjects.h"
#include "rspan.h"

namespace VKKit {
class Device;
class CommandPool;

// A Vulkan texture
class Texture {
public:
    Texture() = default;
    
    /**
     * @brief Construct a texture
     * 
     * @param physical_device The physical device that uses the texture
     * @param device The logical device that uses the texture
     * @param pool The command pool where the texture operations will be executed
     * @param graphics_queue The queue where the drawing commands will occur
     * @param format The texture format
     * @param tiling The texture's tiling properties
     * @param usage What the texture will be used for
     * @param properties The texture's memory properties
     * @param mip_levels The amount of mip levels to create. Set to 1 to create no mip_maps, set to 0 to generate as many mip maps as possible
     * (each half the size of the previous)
     * @param filepath The file from which to load the texture's image
     * @param alpha Whether the image has transparency
     * 
     * @throw std::runtime_error with error info if texture construction fails
     */
    Texture(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, VkFormat format, VkImageAspectFlags aspect,
        VkImageTiling tiling, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, uint32_t mip_levels,
        std::string_view filepath, bool alpha);
    
    Texture(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, VkFormat format, uint32_t width, uint32_t height,
        VkImageAspectFlags aspect, VkImageTiling tiling, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
        uint32_t mip_levels);

    Texture(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, VkFormat format, uint32_t width, uint32_t height,
        ut::rspan<const unsigned char> image_data, VkImageTiling tiling, VkSampleCountFlagBits samples, uint32_t mips);

    VkImage GetTexture() const noexcept { return texture.Get(); }
    VkDeviceMemory GetMemory() const noexcept { return memory.Get(); }
    VkImageView GetView() const noexcept { return view.Get(); }

    int GetWidth() const noexcept { return width; }
    int GetHeight() const noexcept { return height; }
    int GetSize() const noexcept { return width * height * sizeof(int); }
    uint32_t GetMipmaps() const noexcept { return mipmap_levels; }

private:
    Image texture;
    DeviceMemory memory;
    ImageView view;
    uint32_t width, height, channels;
    uint32_t mipmap_levels;
};
}

#endif