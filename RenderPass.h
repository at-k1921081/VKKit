#ifndef VULKANRENDERPASS_H
#define VULKANRENDERPASS_H

#include <span>
#include "vulkan/vulkan.hpp"

namespace VKKit {
class Device;

// A Vulkan render pass. Wrapper around VkRenderPass.
class RenderPass {
public:
    RenderPass() noexcept;

    /**
     * @brief Construct a render pass
     * 
     * @param device The device which will use the render pass
     * @param attachments The attachment descriptions. Describes the rendering stages (sampling, drawing, depth testing, etc.)
     * @param subpass The subpass description. Describes which part of the pipeline the pass will work with, as well as the references for the stages
     * @param dependency Any other render passes that this one will depend on
     * 
     * @throw std::runtime_error with error information on failure
     */
    RenderPass(VkDevice device, std::span<const VkAttachmentDescription> attachments, const VkSubpassDescription& subpass,
        const VkSubpassDependency& dependency);

    /**
     * @brief Construct a render pass
     * 
     * @param device The device which will use the render pass
     * @param attachments The attachment descriptions. Describes the rendering stages (sampling, drawing, depth testing, etc.)
     * @param subpass The subpass description. Describes which part of the pipeline the pass will work with, as well as the references for the stages
     * @param dependency Any other render passes that this one will depend on
     * 
     * @throw std::runtime_error with error information on failure
     */
    RenderPass(const Device& device, std::span<const VkAttachmentDescription> attachments, const VkSubpassDescription& subpass,
        const VkSubpassDependency& dependency);

    ~RenderPass();

    RenderPass(const RenderPass& rp) = delete;
    RenderPass& operator=(const RenderPass& rp) = delete;
    RenderPass(RenderPass&& rp) noexcept;
    RenderPass& operator=(RenderPass&& rp) noexcept;

    VkRenderPass Get() const noexcept { return render_pass; }

private:
    VkDevice device;
    VkRenderPass render_pass;
};
}

#endif