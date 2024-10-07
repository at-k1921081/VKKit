#include <vector>

#include "Filebuf.h"

#include "VkResultString.h"
#include "GraphicsPipeline.h"

namespace {
class Shader {
public:
    Shader() noexcept :
        device{ nullptr }, shader_module{ nullptr }
    {}

    Shader(VkDevice device, std::string_view filepath) :
        device{ device }
    {
        ut::Filebuf code(filepath.data());

        VkShaderModuleCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = code.Size();
        info.pCode = reinterpret_cast<const uint32_t*>(code.Get());

        const auto result = vkCreateShaderModule(device, &info, nullptr, &shader_module);
        if (result != VK_SUCCESS) VKKit::ThrowError("Failed to create shader module.", result);
    }

    ~Shader()
    {
        if (device) vkDestroyShaderModule(device, shader_module, nullptr);
    }

    Shader(const Shader& s) = delete;
    Shader& operator=(const Shader& s) = delete;
    Shader(Shader&& s) noexcept :
        device{ s.device }, shader_module{ s.shader_module }
    {
        s.device = nullptr;
        s.shader_module = nullptr;
    }
    Shader& operator=(Shader&& s) noexcept
    {
        if (device) vkDestroyShaderModule(device, shader_module, nullptr);
        device = s.device;
        s.device = nullptr;
        shader_module = s.shader_module;
        s.shader_module = nullptr;
        return *this;
    }

    VkShaderModule Get() const noexcept { return shader_module; }

private:
    VkDevice device;
    VkShaderModule shader_module;
};
}

namespace VKKit {
GraphicsPipeline::GraphicsPipeline() noexcept :
    device{ nullptr }, layout{ nullptr }, pipeline{ nullptr }
{}

GraphicsPipeline::GraphicsPipeline(
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
) :
    device{ device }
{
    (void)physical_device;

    const Shader vertex_shader(device, vertex_shader_file);
    const Shader fragment_shader(device, fragment_shader_file);

    enum Shaders { VERTEX, FRAGMENT, TOTAL };

    const std::array<VkPipelineShaderStageCreateInfo, Shaders::TOTAL> stages = {
        // Vertex shader
        VkPipelineShaderStageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader.Get(),
            .pName = "main"
        },
        // Fragment shader
        VkPipelineShaderStageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_shader.Get(),
            .pName = "main"
        }
    };

    const VkPipelineDynamicStateCreateInfo dynamic_states_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data()
    };

    const auto layout_result = vkCreatePipelineLayout(device, &pipeline_layout, nullptr, &layout);
    if (layout_result != VK_SUCCESS) ThrowError("Failed to create pipeline layout.", layout_result);

    const VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(stages.size()),
        .pStages = stages.data(),
        .pVertexInputState = &vertex_input_state,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_states_info,
        .layout = layout,
        .renderPass = render_pass,
        .subpass = subpass
    };

    const auto result_pipeline = vkCreateGraphicsPipelines(device, nullptr, 1, &pipeline_info, nullptr, &pipeline);
    if (result_pipeline != VK_SUCCESS) {
        vkDestroyPipelineLayout(device, layout, nullptr);
        ThrowError("Failed to create graphics pipeline.", result_pipeline);
    }
}

GraphicsPipeline::~GraphicsPipeline()
{
    if (device) {
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, layout, nullptr);
    }
}

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& gp) noexcept :
    device{ gp.device }, layout{ gp.layout }, pipeline{ gp.pipeline }
{
    gp.device = nullptr;
    gp.layout = nullptr;
    gp.pipeline = nullptr;
}

GraphicsPipeline& GraphicsPipeline::operator=(GraphicsPipeline&& gp) noexcept
{
    if (device) {
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, layout, nullptr);
    }

    device = gp.device;
    layout = gp.layout;
    pipeline = gp.pipeline;
    gp.device = nullptr;
    gp.layout = nullptr;
    gp.pipeline = nullptr;

    return *this;
}
}