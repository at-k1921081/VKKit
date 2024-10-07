#include "RenderPass.h"
#include "VkResultString.h"
#include "Device.h"

namespace VKKit {
RenderPass::RenderPass() noexcept :
    device{ nullptr }, render_pass{ nullptr }
{}

RenderPass::RenderPass(VkDevice device, std::span<const VkAttachmentDescription> attachments,
    const VkSubpassDescription& subpass, const VkSubpassDependency& dependency) :
    device{ device }
{
    const VkRenderPassCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    const auto result = vkCreateRenderPass(device, &info, nullptr, &render_pass);
    if (result != VK_SUCCESS) ThrowError("Failed to create render pass.", result);
}

RenderPass::RenderPass(const Device& device, std::span<const VkAttachmentDescription> attachments, const VkSubpassDescription& subpass,
    const VkSubpassDependency& dependency) :
    RenderPass(device.Get(), attachments, subpass, dependency)
{}

RenderPass::~RenderPass()
{
    if (device) vkDestroyRenderPass(device, render_pass, nullptr);
}

RenderPass::RenderPass(RenderPass&& rp) noexcept :
    device{ rp.device }, render_pass{ rp.render_pass }
{
    rp.device = nullptr;
    rp.render_pass = nullptr;
}

RenderPass& RenderPass::operator=(RenderPass&& rp) noexcept
{
    if (device) vkDestroyRenderPass(device, render_pass, nullptr);
    device = rp.device;
    render_pass = rp.render_pass;
    rp.device = nullptr;
    rp.render_pass = nullptr;
    return *this;
}
}