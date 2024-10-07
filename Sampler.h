#ifndef VULKANSAMPLER_H
#define VULKANSAMPLER_H

#include "vulkan/vulkan.hpp"

namespace VKKit {
// A Vulkan sampler, used for reading image data and applying filtering and transformation to the texture. Wrapper over VkSampler.
class Sampler {
public:
    Sampler() noexcept;

    /**
     * @brief Construct a sampler
     * 
     * @param device The device where the sampler will be used
     * @param flags The flags for sampler creation
     * @param magFilter Magnification filter
     * @param minFilter Minification filter
     * @param mipmapMode How the sampler handles mipmaps
     * @param addressModeU How rendering outside the [0,1) coordinates on the U axis will work
     * @param addressModeV How rendering outside the [0,1) coordinates on the V axis will work
     * @param addressModeW How rendering outside the [0,1) coordinates on the W axis will work
     * @param mipLodBias The bias added to mipmap level of detail (LOD) calculation
     * @param anisotropyEnable Whether anisotropic filtering is enabled. Used for things like anti-aliasing.
     * @param maxAnisotropy The anisotropy value clamp
     * @param compareEnable Enable comparison against a reference value during lookups
     * @param compareOp The operation which will be used for comparison to fetched data
     * @param minLod Minimum computed LOD
     * @param maxLod Maximum computed LOD
     * @param borderColor The color of the border around images
     * @param unnormalizedCoordinates Whether the coordinates of the image are normalized or not. If they are, the values to access their coordinates
     * are [0, 1). Otherwise they are accessed by their pixels.
     */
    Sampler(
        VkDevice device,
        VkSamplerCreateFlags    flags,
        VkFilter                magFilter,
        VkFilter                minFilter,
        VkSamplerMipmapMode     mipmapMode,
        VkSamplerAddressMode    addressModeU,
        VkSamplerAddressMode    addressModeV,
        VkSamplerAddressMode    addressModeW,
        float                   mipLodBias,
        VkBool32                anisotropyEnable,
        float                   maxAnisotropy,
        VkBool32                compareEnable,
        VkCompareOp             compareOp,
        float                   minLod,
        float                   maxLod,
        VkBorderColor           borderColor,
        VkBool32                unnormalizedCoordinates
    );

    ~Sampler();

    Sampler(const Sampler& s) = delete;
    Sampler& operator=(const Sampler& s) = delete;
    Sampler(Sampler&& s) noexcept;
    Sampler& operator=(Sampler&& s) noexcept;

    VkSampler Get() const noexcept { return sampler; }

private:
    VkDevice device;
    VkSampler sampler;
};
}

#endif