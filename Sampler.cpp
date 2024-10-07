#include "Sampler.h"
#include "VkResultString.h"

namespace VKKit {
Sampler::Sampler() noexcept :
    device{ nullptr }, sampler{ nullptr }
{}

Sampler::Sampler(
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
    VkBool32                unnormalizedCoordinates) :
    device{ device }
{
    const VkSamplerCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .flags = flags,
        .magFilter = magFilter,
        .minFilter = minFilter,
        .mipmapMode = mipmapMode,
        .addressModeU = addressModeU,
        .addressModeV = addressModeV,
        .addressModeW = addressModeW,
        .mipLodBias = mipLodBias,
        .anisotropyEnable = anisotropyEnable,
        .maxAnisotropy = maxAnisotropy,
        .compareEnable = compareEnable,
        .compareOp = compareOp,
        .minLod = minLod,
        .maxLod = maxLod,
        .borderColor = borderColor,
        .unnormalizedCoordinates = unnormalizedCoordinates
    };

    const auto result = vkCreateSampler(device, &info, nullptr, &sampler);
    if (result != VK_SUCCESS) ThrowError("Failed to create sampler.", result);
}

Sampler::~Sampler()
{
    if (device) vkDestroySampler(device, sampler, nullptr);
}

Sampler::Sampler(Sampler&& s) noexcept :
    device{ s.device }, sampler{ s.sampler }
{
    s.device = nullptr;
    s.sampler = nullptr;
}

Sampler& Sampler::operator=(Sampler&& s) noexcept
{
    if (device) vkDestroySampler(device, sampler, nullptr);
    device = s.device;
    sampler = s.sampler;
    s.device = nullptr;
    s.sampler = nullptr;
    return *this;
}
}