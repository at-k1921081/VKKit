#ifndef GRAPHICSPIPELINE_H
#define GRAPHICSPIPELINE_H

#include <span>
#include <string_view>
#include "vulkan/vulkan.hpp"

namespace VKKit {
// Graphics pipeline, which contains all the settings and stages to render on the screen. Wrapper over VkPipelineLayout and VkPipeline.
class GraphicsPipeline {
public:
    GraphicsPipeline() noexcept;

    /**
     * @brief Construct a graphics pipeline
     * 
     * @param physical_device The physical device that will work with the pipeline.
     * @param device The logical device which will use the pipeline.
     * @param vertex_shader_file Path to the file with vertex shader spv code.
     * @param fragment_shader_file Path to the file with fragment shader spv code.
     * @param vertex_input_state Describes the layout of the vertices that will be sent to the pipeline. This includes what type of data is sent, what its
     * offset is and what the stride/size of a vertex is.
     * @param input_assembly How the pipeline assembles the input sent to it. This includes the drawing type (triangles, lines, points) and whether primitive
     * restarting (when an index during indexed drawing can have a special value that resets the assembly of primitives).
     * @param viewport_state The state of the viewport, which states what part of the screen to render to (the viewport) and what part of the image to render
     * (the scissor).
     * @param rasterizer The rasterization settings, which include things like fill mode, cull mode, depth bias, etc.
     * @param multisampling Whether multisampling is used, and how many samples per fragment to use. Used for anti-aliasing.
     * @param depth_stencil Whether the pipeline will do a depth and stencil test, and their properties.
     * @param color_blending Whether color blending is enabled. Used to implement and control transparency.
     * @param pipeline_layout What data bindings the pipeline will have, and at what stages are they found.
     * @param dynamic_states Most operation settings of a graphics pipeline in Vulkan is immutable. This argument describes which of them will be mutable.
     * @param render_pass The render pass that the pipeline will be using
     * @param subpass The index of the subpass in the render pass that this graphics pipeline will be using.
     * 
     * @throw std::runtime_error with error information on failure
     */
    GraphicsPipeline(
        VkPhysicalDevice physical_device,
        VkDevice device,
        std::string_view vertex_shader_file,
        std::string_view fragment_shader_file,
        const VkPipelineVertexInputStateCreateInfo& vertex_input_state,
        const VkPipelineInputAssemblyStateCreateInfo& input_assembly,
        const VkPipelineViewportStateCreateInfo& viewport_state,
        const VkPipelineRasterizationStateCreateInfo& rasterizer,
        const VkPipelineMultisampleStateCreateInfo& multisampling,
        const VkPipelineDepthStencilStateCreateInfo& depth_stencil,
        const VkPipelineColorBlendStateCreateInfo& color_blending,
        const VkPipelineLayoutCreateInfo& pipeline_layout,
        std::span<const VkDynamicState> dynamic_states,
        VkRenderPass render_pass,
        uint32_t subpass
    );
    ~GraphicsPipeline();

    GraphicsPipeline(const GraphicsPipeline& gp) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline& gp) = delete;
    GraphicsPipeline(GraphicsPipeline&& gp) noexcept;
    GraphicsPipeline& operator=(GraphicsPipeline&& gp) noexcept;

    VkPipelineLayout GetLayout() const noexcept { return layout; }
    VkPipeline GetPipeline() const noexcept { return pipeline; }

private:
    VkDevice device;
    VkPipelineLayout layout;
    VkPipeline pipeline;
};
}

#endif