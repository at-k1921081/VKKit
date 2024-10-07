#include <chrono>
#include <thread>
#include <map>
#include <set>
#include <limits>
#include <algorithm>
#include <iostream>
#include <optional>
#include <chrono>
#include <array>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "SDL_vulkan.h"
#include "SDL.h"
#include "vulkan/vulkan.h"

#include "Filebuf.h"

#include "Context.h"
#include "Constants.h"
#include "VkResultString.h"
#include "Debugger.h"
#include "CommandBuffer.h"
#include "PhysicalDevice.h"
#include "DefaultConfigurations.h"
#include "Alphabet.h"
#include "Windowing.h"
#include "Concurrency.h"
#include "Buffer.h"
#ifndef NDEBUG
#include "Debugger.h"
#endif
#include "Texture.h"
#include "Model.h"
#include "GraphicsPipeline.h"
#include "VulkanObjects.h"
#include "Swapchain.h"
#include "CommandBuffer.h"
#include "Constants.h"
#include "InitLibs.h"

using namespace std::chrono;

namespace VKKit {
#ifndef NDEBUG
static constexpr std::array<const char*, 1> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
};
#endif

class Context::Impl {
public:
    Impl(std::string_view window_title, int screenw, int screenh);
    Impl(std::string_view window_title);
    Impl(void* native_handle, bool drop);
    ~Impl();

    bool BeginRendering();
    void EndRendering();

    void Render2D(size_t texture, Rect dst);
    void Render2D(size_t texture, Rect src, Rect dst);
    void Color2D(Color color, Rect area);
    void Render3D(size_t texture, Cuboid area, const CameraView& camera);
    void Render3D(size_t texture, const Model& model, const CameraView& camera);
    void Color3D(Color color, Cuboid area, const CameraView& camera);
    void RenderTextRel(std::string_view text, size_t font_style, Color color, float x, float y, float size, float row_width = std::numeric_limits<float>::max(),
        HorizontalAlignment halign = HorizontalAlignment::LEFT, VerticalAlignment valign = VerticalAlignment::TOP);
    void RenderTextAbs(std::string_view text, size_t font_style, Color color, float x, float y, float size, float row_width = std::numeric_limits<float>::max(),
        HorizontalAlignment halign = HorizontalAlignment::LEFT, VerticalAlignment valign = VerticalAlignment::TOP);

    // Load a texture from path and append to the array of textures
    void LoadTexture(std::string_view path);
    void LoadTextures(std::span<const std::string_view> paths);

    void LoadAlphabet(std::string_view path);
    void LoadAlphabets(std::span<const std::string_view> paths);

    const Window& GetWindow() const noexcept { return window; }
    VkExtent2D GetSwapchainExtent() const noexcept { return swapchain.GetExtent(); }
    Rect GetWindowRect() const noexcept;

    void SetFramebufferResized() noexcept { framebuffer_resized = true; }

    void SetParentWindow(void* native_handle);

private:
    Window window;
    FreeType freetype;
    
    Instance instance;
#ifndef NDEBUG
    Debugger debugger;
#endif
    Surface surface;
    VkPhysicalDevice physical_device;
    VkSampleCountFlagBits msaa;
    Device device;
    RenderPass render_pass;

    enum class GraphicsPipelines { COLOR2D, COLOR3D, TEXTURE2D, TEXTURE3D, TEXT, TOTAL_PIPELINES };

    std::array<DescriptorSetLayout, static_cast<size_t>(GraphicsPipelines::TOTAL_PIPELINES)> descriptor_set_layouts;
    Swapchain swapchain;
    std::array<GraphicsPipeline, static_cast<size_t>(GraphicsPipelines::TOTAL_PIPELINES)> graphics_pipelines;
    CommandPool command_pool;
    std::vector<CommandBuffer> command_buffers;
    std::vector<Semaphore> image_available_semaphores;
    std::vector<Semaphore> render_finished_semaphores;
    std::vector<Fence> in_flight_fences;

    std::vector<Texture> textures;
    std::vector<VkDescriptorSet> texture_sets_2d;
    std::vector<VkDescriptorSet> texture_sets_3d;

    std::vector<Alphabet> alphabets;

    std::array<std::vector<Buffer>, MAX_FRAMES_IN_FLIGHT> vertex_buffers;
    std::array<std::vector<Buffer>, MAX_FRAMES_IN_FLIGHT> vertex_staging_buffers;
    std::array<std::vector<Buffer>, MAX_FRAMES_IN_FLIGHT> index_buffers;
    std::array<std::vector<Buffer>, MAX_FRAMES_IN_FLIGHT> index_staging_buffers;
    std::array<size_t, MAX_FRAMES_IN_FLIGHT> current_buffer_positions;

    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> color_sets;

    enum class TextureDescriptorSets { TEXTURE2D, TEXTURE3D, TOTAL };
    std::array<VkDescriptorSet, static_cast<size_t>(TextureDescriptorSets::TOTAL) * MAX_FRAMES_IN_FLIGHT> texture_sets;

    enum class UniformBuffers { COLOR3D, TEXTURE3D, TEXT_PROJECTION, TOTAL };
    std::array<Buffer, static_cast<size_t>(UniformBuffers::TOTAL) * MAX_FRAMES_IN_FLIGHT> uniform_buffers;
    std::array<void*, static_cast<size_t>(UniformBuffers::TOTAL) * MAX_FRAMES_IN_FLIGHT> uniform_buffers_mapped;
    
    DescriptorPool descriptor_pool;
    Sampler sampler;
    
    std::vector<Model> models;

    uint32_t current_frame;
    bool framebuffer_resized;
    std::chrono::high_resolution_clock::time_point time;
    uint32_t image_index;

    void CreateSwapchain();
    void CreateRenderPass();
    void CreateDescriptorLayout();
    void CreatePipelines();
    void CreateCommandPool();
    void CreateTextureSampler();
    
    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateBuiltinDescriptorSets();
    void CreateCommandBuffers();
    void CreateSyncObjects();

    void RecreateSwapChain();
    void AddDescriptorSet2D(const Texture& texture);
    void AddDescriptorSet3D(const Texture& texture);
};

Context::Impl::Impl(std::string_view window_title, int screenw, int screenh) :
    window{ window_title, screenw, screenh },
#ifndef NDEBUG
    instance{ window_title, VALIDATION_LAYERS, window },
    debugger{ instance.Get() },
#else
    instance{ window_title, window },
#endif
    surface{ instance.Get(), window },
    physical_device{ PickPhysicalDevice(instance, surface) },
    msaa{ GetMaxUsableSampleCount(physical_device) }, 
#ifndef NDEBUG
    device{ physical_device, VkPhysicalDeviceFeatures { .sampleRateShading = VK_TRUE, .samplerAnisotropy = VK_TRUE }, surface.Get(),
        DEVICE_EXTENSIONS, VALIDATION_LAYERS },
#else
    device{ Device(physical_device, VkPhysicalDeviceFeatures { .sampleRateShading = VK_TRUE, .samplerAnisotropy = VK_TRUE }, surface.Get(),
        DEVICE_EXTENSIONS) },
#endif
    current_frame{ 0 }, framebuffer_resized{ false },
    time{ high_resolution_clock::now() },
    image_index{ 0 },
    current_buffer_positions{}
{
    CreateRenderPass();
    CreateDescriptorLayout();
    CreateSwapchain();
    CreatePipelines();
    CreateCommandPool();
    CreateTextureSampler();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateBuiltinDescriptorSets();
    CreateCommandBuffers();
    CreateSyncObjects();
}

Context::Impl::Impl(std::string_view window_title) :
    window{ window_title },
#ifndef NDEBUG
    instance{ window_title, VALIDATION_LAYERS, window },
    debugger{ instance.Get() },
#else
    instance{ window_title, window },
#endif
    surface{ instance.Get(), window },
    physical_device{ PickPhysicalDevice(instance, surface) },
    msaa{ GetMaxUsableSampleCount(physical_device) }, 
#ifndef NDEBUG
    device{ physical_device, VkPhysicalDeviceFeatures { .sampleRateShading = VK_TRUE, .samplerAnisotropy = VK_TRUE }, surface.Get(),
        DEVICE_EXTENSIONS, VALIDATION_LAYERS },
#else
    device{ Device(physical_device, VkPhysicalDeviceFeatures { .sampleRateShading = VK_TRUE, .samplerAnisotropy = VK_TRUE }, surface.Get(),
        DEVICE_EXTENSIONS) },
#endif
    current_frame{ 0 }, framebuffer_resized{ false },
    time{ high_resolution_clock::now() },
    image_index{ 0 },
    current_buffer_positions{}
{
    CreateRenderPass();
    CreateDescriptorLayout();
    CreateSwapchain();
    CreatePipelines();
    CreateCommandPool();
    CreateTextureSampler();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateBuiltinDescriptorSets();
    CreateCommandBuffers();
    CreateSyncObjects();
}

Context::Impl::Impl(void* native_handle, bool drop) :
    window{ native_handle, drop },
#ifndef NDEBUG
    instance{ "", VALIDATION_LAYERS, window },
    debugger{ instance.Get() },
#else
    instance{ "", window},
#endif
    surface{ instance.Get(), window },
    physical_device{ PickPhysicalDevice(instance, surface) },
    msaa{ GetMaxUsableSampleCount(physical_device) }, 
#ifndef NDEBUG
    device{ physical_device, VkPhysicalDeviceFeatures { .sampleRateShading = VK_TRUE, .samplerAnisotropy = VK_TRUE }, surface.Get(),
        DEVICE_EXTENSIONS, VALIDATION_LAYERS },
#else
    device{ Device(physical_device, VkPhysicalDeviceFeatures { .sampleRateShading = VK_TRUE, .samplerAnisotropy = VK_TRUE }, surface.Get(),
        DEVICE_EXTENSIONS) },
#endif
    current_frame{ 0 }, framebuffer_resized{ false },
    time{ high_resolution_clock::now() },
    image_index{ 0 },
    current_buffer_positions{}
{
    CreateRenderPass();
    CreateDescriptorLayout();
    CreateSwapchain();
    CreatePipelines();
    CreateCommandPool();
    CreateTextureSampler();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateBuiltinDescriptorSets();
    CreateCommandBuffers();
    CreateSyncObjects();
}

Context::Impl::~Impl()
{
    device.Wait();
}

void Context::Impl::CreateSwapchain()
{
    swapchain = Swapchain(physical_device, device, command_pool, surface, window, render_pass,
        ChooseSurfaceFormat(physical_device, surface.Get()), VK_PRESENT_MODE_FIFO_KHR, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_TRUE, msaa, nullptr);
}

void Context::Impl::CreateRenderPass()
{
    const auto depth_format = FindDepthFormat(physical_device);
    if (!depth_format) ThrowError("Failed to find depth format.", depth_format.error());

    const auto swapchain_image_format = ChooseSurfaceFormat(physical_device, surface.Get()).format;

    const std::array<VkAttachmentDescription, 3> attachments = {
        // Color attachment
        VkAttachmentDescription {
            .format = swapchain_image_format,
            .samples = msaa,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        },
        // Color attachment resolve
        VkAttachmentDescription {
            .format = swapchain_image_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        },
        // Depth attachment
        VkAttachmentDescription {
            .format = *depth_format,
            .samples = msaa,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        }
    };

    const VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    const VkAttachmentReference color_attachment_resolve_ref = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    const VkAttachmentReference depth_attachment_ref = {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    const VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
        .pResolveAttachments = &color_attachment_resolve_ref,
        .pDepthStencilAttachment = &depth_attachment_ref
    };

    const VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };

    render_pass = RenderPass(device, attachments, subpass, dependency);
}

void Context::Impl::CreateDescriptorLayout()
{
    descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::COLOR2D)] = CreateColor2DLayout(device);
    descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::COLOR3D)] = CreateColor3DLayout(device);
    descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::TEXTURE2D)] = CreateTexture2DLayout(device);
    descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::TEXTURE3D)] = CreateTexture3DLayout(device);
    descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::TEXT)] = CreateTextLayout(device);
}

void Context::Impl::CreatePipelines()
{
    graphics_pipelines[static_cast<size_t>(GraphicsPipelines::COLOR2D)] = CreateColor2DPipeline(physical_device, device, render_pass, swapchain,
        descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::COLOR2D)], msaa);

    graphics_pipelines[static_cast<size_t>(GraphicsPipelines::COLOR3D)] = CreateColor3DPipeline(physical_device, device, render_pass, swapchain,
        descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::COLOR3D)], msaa);

    graphics_pipelines[static_cast<size_t>(GraphicsPipelines::TEXTURE2D)] = CreateTexture2DPipeline(physical_device, device, render_pass, swapchain,
        descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::TEXTURE2D)], msaa);
    
    graphics_pipelines[static_cast<size_t>(GraphicsPipelines::TEXTURE3D)] = CreateTexture3DPipeline(physical_device, device, render_pass, swapchain,
        descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::TEXTURE3D)], msaa);

    graphics_pipelines[static_cast<size_t>(GraphicsPipelines::TEXT)] = CreateTextPipeline(physical_device, device, render_pass, swapchain,
        descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::TEXT)], msaa);
}

void Context::Impl::CreateCommandPool()
{
    command_pool = CommandPool(device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, device.GetGraphicsQueueIndex());
}

void Context::Impl::CreateTextureSampler()
{
    sampler = Sampler(device.Get(), VkSamplerCreateFlags{}, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
        VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, 0.0f, VK_TRUE,
        GetPhysicalDeviceProperties(physical_device).limits.maxSamplerAnisotropy, VK_FALSE, VK_COMPARE_OP_ALWAYS, 0.0f, 5.0f, VkBorderColor{}, VK_FALSE);
}

void Context::Impl::CreateUniformBuffers()
{
    assert(uniform_buffers.size() == uniform_buffers_mapped.size());
    for (size_t i = 0; i < uniform_buffers.size(); ++i) {
        uniform_buffers[i] = Buffer(physical_device, device.Get(), sizeof(CameraView), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_SHARING_MODE_EXCLUSIVE); // FIX THIS
        vkMapMemory(device.Get(), uniform_buffers[i].GetMemory(), 0, sizeof(CameraView), 0, &uniform_buffers_mapped[i]);
    }

    const glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(swapchain.GetWidth()), static_cast<float>(swapchain.GetHeight()), 0.0f);
    // projection[1][1] *= -1;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        memcpy(uniform_buffers_mapped[static_cast<size_t>(UniformBuffers::TEXT_PROJECTION) * MAX_FRAMES_IN_FLIGHT + i], &projection, sizeof(projection));
}

void Context::Impl::CreateDescriptorPool()
{
    const std::array<VkDescriptorPoolSize, 2> pool_sizes = {
        VkDescriptorPoolSize {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 3
        },
        VkDescriptorPoolSize {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 200 // TO DO: Use a dedicated, resizable descriptor pool to store texture views
        }
    };

    descriptor_pool = DescriptorPool(device, {}, pool_sizes, MAX_FRAMES_IN_FLIGHT);
}

void Context::Impl::CreateBuiltinDescriptorSets()
{
    const auto color3d_set = CreateColor3DSet(device, descriptor_pool, descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::COLOR3D)],
        { &uniform_buffers[static_cast<size_t>(UniformBuffers::COLOR3D) * MAX_FRAMES_IN_FLIGHT], MAX_FRAMES_IN_FLIGHT });

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        color_sets[i] = color3d_set;
    }
}

void Context::Impl::CreateCommandBuffers()
{
    command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (auto& c : command_buffers) c = CommandBuffer(device.Get(), command_pool.Get(), VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

void Context::Impl::CreateSyncObjects()
{
    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        image_available_semaphores[i] = Semaphore(device.Get());
        render_finished_semaphores[i] = Semaphore(device.Get());
        in_flight_fences[i] = Fence(device.Get());
    }
}

void Context::Impl::RecreateSwapChain()
{
    int w, h;
    SDL_Vulkan_GetDrawableSize(window.Get(), &w, &h);
    while (w == 0 || h == 0) {
        SDL_Vulkan_GetDrawableSize(window.Get(), &w, &h);
        SDL_WaitEvent(nullptr);
    }

    vkDeviceWaitIdle(device.Get());

    swapchain = Swapchain(physical_device, device, command_pool, surface, window, render_pass,
        ChooseSurfaceFormat(physical_device, surface.Get()), VK_PRESENT_MODE_FIFO_KHR, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_TRUE, msaa, swapchain.Get());

    const glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(swapchain.GetWidth()), static_cast<float>(swapchain.GetHeight()), 0.0f);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        memcpy(uniform_buffers_mapped[static_cast<size_t>(UniformBuffers::TEXT_PROJECTION) * MAX_FRAMES_IN_FLIGHT + i], &projection, sizeof(projection));
}

bool Context::Impl::BeginRendering()
{
    in_flight_fences[current_frame].Wait();

    const auto ac_result = vkAcquireNextImageKHR(device.Get(), swapchain.Get(), UINT64_MAX, image_available_semaphores[current_frame].Get(),
        VK_NULL_HANDLE, &image_index);

    if (ac_result == VK_ERROR_OUT_OF_DATE_KHR || ac_result == VK_SUBOPTIMAL_KHR) {
        RecreateSwapChain();
        return false;
    }
    else if (ac_result != VK_SUCCESS) ThrowError("Failed to acquire next image.", ac_result);

    in_flight_fences[current_frame].Reset();

    current_buffer_positions[current_frame] = 0;
    /*vertex_buffers[current_frame].clear();
    index_buffers[current_frame].clear();
    vertex_staging_buffers[current_frame].clear();
    index_staging_buffers[current_frame].clear();*/

    vkResetCommandBuffer(command_buffers[current_frame].GetBuffer(), 0);
    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = nullptr
    };

    const auto result = vkBeginCommandBuffer(command_buffers[current_frame].GetBuffer(), &begin_info);
    if (result != VK_SUCCESS) ThrowError("Failed to begin recording command buffer.", result);

    static constexpr std::array<VkClearValue, 3> clear_colors = {
        VkClearValue { .color = { { 0.0f, 0.0f, 0.0f, 1.0f } } },
        VkClearValue { .color = { { 0.0f, 0.0f, 0.0f, 1.0f } } },
        VkClearValue { .depthStencil = VkClearDepthStencilValue{ 1.0f, 0 } }
    };

    const VkRenderPassBeginInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass.Get(),
        .framebuffer = swapchain.GetFramebuffers()[image_index].Get(),
        .renderArea = { .offset = { 0, 0 }, .extent = swapchain.GetExtent() },
        .clearValueCount = static_cast<uint32_t>(clear_colors.size()),
        .pClearValues = clear_colors.data()
    };

    vkCmdBeginRenderPass(command_buffers[current_frame].GetBuffer(), &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    const VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(swapchain.GetWidth()),
        .height = static_cast<float>(swapchain.GetHeight()),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(command_buffers[current_frame].GetBuffer(), 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = swapchain.GetExtent()
    };
    vkCmdSetScissor(command_buffers[current_frame].GetBuffer(), 0, 1, &scissor);

    return true;
}

void Context::Impl::EndRendering()
{
    vkCmdEndRenderPass(command_buffers[current_frame].GetBuffer());

    const auto buf_result = vkEndCommandBuffer(command_buffers[current_frame].GetBuffer());
    if (buf_result != VK_SUCCESS) ThrowError("Failed to record command buffer.", buf_result);

    const auto img_av_s = image_available_semaphores[current_frame].Get();
    const auto ren_fin_s = render_finished_semaphores[current_frame].Get();

    const std::array<VkPipelineStageFlags, 1> wait_stages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    const auto cb = command_buffers[current_frame].GetBuffer();
    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &img_av_s,
        .pWaitDstStageMask = wait_stages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &cb,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &ren_fin_s
    };

    const auto submit_result = vkQueueSubmit(device.GetGraphicsQueue(), 1, &submit_info, in_flight_fences[current_frame].Get());
    if (submit_result != VK_SUCCESS) ThrowError("Failed to submit draw command buffer.", submit_result);

    const auto sc = swapchain.Get();
    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &ren_fin_s,
        .swapchainCount = 1,
        .pSwapchains = &sc,
        .pImageIndices = &image_index,
        .pResults = nullptr
    };

    const auto present_result = vkQueuePresentKHR(device.GetPresentQueue(), &present_info);
    if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
        framebuffer_resized = false;
        RecreateSwapChain();
    }
    else if (present_result != VK_SUCCESS)
        ThrowError("Failed to present queue.", present_result);

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    for (auto& a : alphabets) a.ClearBuffers(current_frame);
}

// TO DO: Optimise this function
void Context::Impl::Render2D(size_t texture, Rect dst)
{
    const std::array<float, 20> vertices = {
        dst.x,          dst.y,          0.0f, 0.0f, 0.0f,
        dst.x + dst.w,  dst.y,          0.0f, 1.0f, 0.0f,
        dst.x + dst.w,  dst.y + dst.h,  0.0f, 1.0f, 1.0f,
        dst.x,          dst.y + dst.h,  0.0f, 0.0f, 1.0f
    };

    static constexpr std::array<uint32_t, 6> indices = {
        0, 1, 2, 2, 3, 0
    };

    auto& bufpos = current_buffer_positions[current_frame];
    auto& vbufferarr = vertex_buffers[current_frame];
    auto& vbufferstaging = vertex_staging_buffers[current_frame];
    auto& ibufferarr = index_buffers[current_frame];
    auto& ibufferstaging = index_staging_buffers[current_frame];
    if (bufpos >= vbufferarr.size() || vbufferarr.size() == 0) {
        vbufferarr.push_back(Buffer::CreateVertexBuffer(physical_device, device, command_pool, vertices));
        vbufferstaging.push_back(CreateStagingBuffer(physical_device, device.Get(), sizeof(vertices)));
        ibufferarr.push_back(Buffer::CreateIndexBuffer(physical_device, device, command_pool, indices));
        ibufferstaging.push_back(CreateStagingBuffer(physical_device, device.Get(), sizeof(indices)));
    }
    else {
        vbufferarr[bufpos].WriteData(device, command_pool, vbufferstaging[bufpos], vertices.data(), sizeof(vertices));
        ibufferarr[bufpos].WriteData(device, command_pool, ibufferstaging[bufpos], indices.data(), sizeof(indices));
    }

    vkCmdBindPipeline(command_buffers[current_frame].GetBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipelines[static_cast<size_t>
        (GraphicsPipelines::TEXTURE2D)].GetPipeline());
    const auto buf = vbufferarr[bufpos].GetBuffer();

    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(command_buffers[current_frame].GetBuffer(), 0, 1, &buf, &offset);
    vkCmdBindIndexBuffer(command_buffers[current_frame].GetBuffer(), ibufferarr[bufpos].GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(command_buffers[current_frame].GetBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipelines[static_cast<size_t>
        (GraphicsPipelines::TEXTURE2D)].GetLayout(), 0, 1, &texture_sets_2d[texture * 2 + current_frame], 0, nullptr);

    vkCmdDrawIndexed(command_buffers[current_frame].GetBuffer(), static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    ++bufpos;
}

void Context::Impl::Render2D(size_t texture, Rect src, Rect dst)
{
    (void)texture, src, dst;
}

void Context::Impl::Color2D(Color color, Rect area)
{
    const std::array<float, 28> vertices = {
        area.x,          area.y,          0.0f, color.r, color.g, color.b, color.a,
        area.x + area.w, area.y,          0.0f, color.r, color.g, color.b, color.a,
        area.x + area.w, area.y + area.h, 0.0f, color.r, color.g, color.b, color.a,
        area.x,          area.y + area.h, 0.0f, color.r, color.g, color.b, color.a
    };

    static constexpr std::array<uint32_t, 6> indices = {
        0, 1, 2, 2, 3, 0
    };

    /*vertex_buffers[current_frame].push_back(Buffer::CreateVertexBuffer(physical_device, device, command_pool, vertices));
    index_buffers[current_frame].push_back(Buffer::CreateIndexBuffer(physical_device, device, command_pool, indices));*/
    auto& bufpos = current_buffer_positions[current_frame];
    auto& vbufferarr = vertex_buffers[current_frame];
    auto& vbufferstaging = vertex_staging_buffers[current_frame];
    auto& ibufferarr = index_buffers[current_frame];
    auto& ibufferstaging = index_staging_buffers[current_frame];
    if (bufpos >= vbufferarr.size() || vbufferarr.size() == 0) {
        vbufferarr.push_back(Buffer::CreateVertexBuffer(physical_device, device, command_pool, vertices));
        vbufferstaging.push_back(CreateStagingBuffer(physical_device, device.Get(), sizeof(vertices)));
        ibufferarr.push_back(Buffer::CreateIndexBuffer(physical_device, device, command_pool, indices));
        ibufferstaging.push_back(CreateStagingBuffer(physical_device, device.Get(), sizeof(indices)));
    }
    else {
        const VkDeviceSize vsize = sizeof(vertices);
        const VkDeviceSize isize = sizeof(indices); // TO DO: Fix this code, has buffer overflow
        vbufferarr[bufpos].WriteData(device, command_pool, vbufferstaging[bufpos], vertices.data(), sizeof(vertices));
        ibufferarr[bufpos].WriteData(device, command_pool, ibufferstaging[bufpos], indices.data(), sizeof(indices));
    }

    vkCmdBindPipeline(command_buffers[current_frame].GetBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipelines[static_cast<size_t>
        (GraphicsPipelines::COLOR2D)].GetPipeline());
    const auto buf = vertex_buffers[current_frame].back().GetBuffer();

    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(command_buffers[current_frame].GetBuffer(), 0, 1, &buf, &offset);
    vkCmdBindIndexBuffer(command_buffers[current_frame].GetBuffer(), index_buffers[current_frame].back().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(command_buffers[current_frame].GetBuffer(), static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void Context::Impl::Render3D(size_t texture, Cuboid area, const CameraView& camera)
{
    const std::array<float, 20 * 6> vertices = {
        // Front
        area.x,          area.y,          area.z,          0.0f, 1.0f,
        area.x + area.w, area.y,          area.z,          1.0f, 1.0f,
        area.x + area.w, area.y + area.h, area.z,          1.0f, 0.0f,
        area.x,          area.y + area.h, area.z,          0.0f, 0.0f,

        // Back
        area.x,          area.y,          area.z + area.d, 0.0f, 1.0f,
        area.x + area.w, area.y,          area.z + area.d, 1.0f, 1.0f,
        area.x + area.w, area.y + area.h, area.z + area.d, 1.0f, 0.0f,
        area.x,          area.y + area.h, area.z + area.d, 0.0f, 0.0f,

        // Top
        area.x,          area.y,          area.z + area.d, 0.0f, 1.0f,
        area.x + area.w, area.y,          area.z + area.d, 1.0f, 1.0f,
        area.x + area.w, area.y,          area.z,          1.0f, 0.0f,
        area.x,          area.y,          area.z,          0.0f, 0.0f,

        // Bottom
        area.x,          area.y + area.h, area.z + area.d, 0.0f, 1.0f,
        area.x + area.w, area.y + area.h, area.z + area.d, 1.0f, 1.0f,
        area.x + area.w, area.y + area.h, area.z,          1.0f, 0.0f,
        area.x,          area.y + area.h, area.z,          0.0f, 0.0f,

        // Left
        area.x,          area.y,          area.z + area.d, 0.0f, 1.0f,
        area.x,          area.y,          area.z,          1.0f, 1.0f,
        area.x,          area.y + area.h, area.z,          1.0f, 0.0f,
        area.x,          area.y + area.h, area.z + area.d, 0.0f, 0.0f,

        // Right
        area.x + area.w, area.y,          area.z + area.d, 0.0f, 1.0f,
        area.x + area.w, area.y,          area.z,          1.0f, 1.0f,
        area.x + area.w, area.y + area.h, area.z,          1.0f, 0.0f,
        area.x + area.w, area.y + area.h, area.z + area.d, 0.0f, 0.0f,
    };

    const std::array<uint32_t, 36> indices = {
        0, 1, 2, 2, 3, 0,       // Front
        4, 5, 6, 6, 7, 4,       // Back
        8, 9, 10, 10, 11, 8,    // Left
        12, 13, 14, 14, 15, 12, // Right
        16, 17, 18, 18, 19, 16, // Top
        20, 21, 22, 22, 23, 20, // Bottom
    };

    vertex_buffers[current_frame].push_back(Buffer::CreateVertexBuffer(physical_device, device, command_pool, vertices));
    index_buffers[current_frame].push_back(Buffer::CreateIndexBuffer(physical_device, device, command_pool, indices));

    vkCmdBindPipeline(command_buffers[current_frame].GetBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipelines[static_cast<size_t>
        (GraphicsPipelines::TEXTURE3D)].GetPipeline());

    const auto vertex_buffer = vertex_buffers[current_frame].back().GetBuffer();
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(command_buffers[current_frame].GetBuffer(), 0, 1, &vertex_buffer, &offset);
    vkCmdBindIndexBuffer(command_buffers[current_frame].GetBuffer(), index_buffers[current_frame].back().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(command_buffers[current_frame].GetBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipelines[static_cast<size_t>
        (GraphicsPipelines::TEXTURE3D)].GetLayout(), 0, 1, &texture_sets_3d[texture * MAX_FRAMES_IN_FLIGHT + current_frame], 0, nullptr);

    memcpy(uniform_buffers_mapped[static_cast<size_t>(UniformBuffers::TEXTURE3D) * 2 + current_frame], &camera, sizeof(CameraView));

    vkCmdDrawIndexed(command_buffers[current_frame].GetBuffer(), static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void Context::Impl::Render3D(size_t texture, const Model& model, const CameraView& camera)
{
    vertex_buffers[current_frame].push_back(Buffer::CreateVertexBuffer(physical_device, device, command_pool, model.GetVertices()));
    index_buffers[current_frame].push_back(Buffer::CreateIndexBuffer(physical_device, device, command_pool, model.GetIndices()));

    vkCmdBindPipeline(command_buffers[current_frame].GetBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipelines[static_cast<size_t>
        (GraphicsPipelines::TEXTURE3D)].GetPipeline());

    const auto vertex_buffer = vertex_buffers[current_frame].back().GetBuffer();
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(command_buffers[current_frame].GetBuffer(), 0, 1, &vertex_buffer, &offset);
    vkCmdBindIndexBuffer(command_buffers[current_frame].GetBuffer(), index_buffers[current_frame].back().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(command_buffers[current_frame].GetBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipelines[static_cast<size_t>
        (GraphicsPipelines::TEXTURE3D)].GetLayout(), 0, 1, &texture_sets_3d[texture * MAX_FRAMES_IN_FLIGHT + current_frame], 0, nullptr);

    memcpy(uniform_buffers_mapped[static_cast<size_t>(UniformBuffers::TEXTURE3D) * 2 + current_frame], &camera, sizeof(CameraView));

    vkCmdDrawIndexed(command_buffers[current_frame].GetBuffer(), static_cast<uint32_t>(model.GetIndices().size()), 1, 0, 0, 0);
}

void Context::Impl::Color3D(Color color, Cuboid area, const CameraView& camera)
{
    const std::array<float, 56> vertices = {
        // Front
        area.x,          area.y,          area.z, color.r, color.g, color.b, color.a,
        area.x + area.w, area.y,          area.z, color.r, color.g, color.b, color.a,
        area.x + area.w, area.y + area.h, area.z, color.r, color.g, color.b, color.a,
        area.x,          area.y + area.h, area.z, color.r, color.g, color.b, color.a,

        // Back
        area.x,          area.y,          area.z + area.d, color.r, color.g, color.b, color.a,
        area.x + area.w, area.y,          area.z + area.d, color.r, color.g, color.b, color.a,
        area.x + area.w, area.y + area.h, area.z + area.d, color.r, color.g, color.b, color.a,
        area.x,          area.y + area.h, area.z + area.d, color.r, color.g, color.b, color.a
    };

    const std::array<uint32_t, 36> indices = {
        0, 1, 2, 2, 3, 0, // Front
        4, 5, 6, 6, 7, 4, // Back
        4, 0, 3, 3, 7, 4, // Left
        1, 5, 6, 6, 2, 1, // Right
        4, 5, 1, 1, 0, 4, // Top
        7, 6, 2, 2, 3, 7, // Bottom
    };

    vertex_buffers[current_frame].push_back(Buffer::CreateVertexBuffer(physical_device, device, command_pool, vertices));
    index_buffers[current_frame].push_back(Buffer::CreateIndexBuffer(physical_device, device, command_pool, indices));

    vkCmdBindPipeline(command_buffers[current_frame].GetBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphics_pipelines[static_cast<size_t>(GraphicsPipelines::COLOR3D)].GetPipeline());

    const auto buf = vertex_buffers[current_frame].back().GetBuffer();
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(command_buffers[current_frame].GetBuffer(), 0, 1, &buf, &offset);
    vkCmdBindIndexBuffer(command_buffers[current_frame].GetBuffer(), index_buffers[current_frame].back().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(command_buffers[current_frame].GetBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipelines[static_cast<size_t>
        (GraphicsPipelines::COLOR3D)].GetLayout(), 0, 1, &color_sets[current_frame],
        0, nullptr);

    memcpy(uniform_buffers_mapped[static_cast<size_t>(UniformBuffers::COLOR3D) * 2 + current_frame], &camera, sizeof(CameraView));

    vkCmdDrawIndexed(command_buffers[current_frame].GetBuffer(), static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void Context::Impl::RenderTextRel(std::string_view text, size_t font_style, Color color, float x, float y, float size, float row_width,
    HorizontalAlignment halign, VerticalAlignment valign)
{
    std::span<const Buffer> projection_uniforms = { &uniform_buffers[static_cast<size_t>(UniformBuffers::TEXT_PROJECTION) * MAX_FRAMES_IN_FLIGHT], MAX_FRAMES_IN_FLIGHT };

    float size_offset = size;
    switch (valign) {
    case VerticalAlignment::TOP: break;
    case VerticalAlignment::CENTER: size_offset = 0.0f; break;
    case VerticalAlignment::BOTTOM: size_offset *= -1.0f; break;
    }

    alphabets[font_style].RenderTextRel(physical_device, device, command_pool, command_buffers[current_frame], graphics_pipelines[static_cast<size_t>
        (GraphicsPipelines::TEXT)], descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::TEXT)], sampler, projection_uniforms, current_frame,
        text, color, size, x, DEFAULT_SCREEN_HEIGHT - y - size_offset, swapchain.GetExtent(), halign, valign, row_width);
}

void Context::Impl::RenderTextAbs(std::string_view text, size_t font_style, Color color, float x, float y, float size, float row_width,
    HorizontalAlignment halign, VerticalAlignment valign)
{
    std::span<const Buffer> projection_uniforms = { &uniform_buffers[static_cast<size_t>(UniformBuffers::TEXT_PROJECTION) * MAX_FRAMES_IN_FLIGHT], MAX_FRAMES_IN_FLIGHT };

    float size_offset = size;
    switch (valign) {
    case VerticalAlignment::TOP: break;
    case VerticalAlignment::CENTER: size_offset = 0.0f; break;
    case VerticalAlignment::BOTTOM: size_offset *= -1.0f; break;
    }

    alphabets[font_style].RenderTextAbs(physical_device, device, command_pool, command_buffers[current_frame], graphics_pipelines[static_cast<size_t>
        (GraphicsPipelines::TEXT)], descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::TEXT)], sampler, projection_uniforms, current_frame,
        text, color, size, x, static_cast<float>(swapchain.GetHeight()) - y - size_offset, halign, valign, row_width);
}

void Context::Impl::LoadTexture(std::string_view path)
{
    if (path.empty()) {
        textures.emplace_back();
        return;
    }

    textures.emplace_back(physical_device, device, command_pool, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_TILING_OPTIMAL,
        VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, path, true);

    AddDescriptorSet2D(textures.back());
    AddDescriptorSet3D(textures.back());

    /*for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vertex_buffers[i].resize(vertex_buffers.size() + 1);
        index_buffers[i].resize(index_buffers.size() + 1);
    }*/
}

void Context::Impl::LoadTextures(std::span<const std::string_view> paths)
{
    textures.resize(textures.size() + paths.size());

    for (const auto p : paths) LoadTexture(p);
}

void Context::Impl::LoadAlphabet(std::string_view path)
{
    // std::span<const Buffer> color_uniforms = { &uniform_buffers[static_cast<size_t>(UniformBuffers::TEXT_COLOR) * MAX_FRAMES_IN_FLIGHT], MAX_FRAMES_IN_FLIGHT };
    std::span<const Buffer> projection_uniforms = { &uniform_buffers[static_cast<size_t>(UniformBuffers::TEXT_PROJECTION) * MAX_FRAMES_IN_FLIGHT], MAX_FRAMES_IN_FLIGHT };

    alphabets.emplace_back(freetype, path, physical_device, device, command_pool, descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::TEXT)],
        sampler, projection_uniforms, VK_SAMPLE_COUNT_1_BIT);
}

void Context::Impl::LoadAlphabets(std::span<const std::string_view> paths)
{
    for (const auto p : paths) LoadAlphabet(p);
}

Rect Context::Impl::GetWindowRect() const noexcept
{
    int x, y;
    SDL_GetWindowPosition(window.Get(), &x, &y);

    const auto extent = GetSwapchainExtent();

    return Rect{ static_cast<float>(x), static_cast<float>(y), static_cast<float>(extent.width), static_cast<float>(extent.height) };
}

void Context::Impl::AddDescriptorSet2D(const Texture& texture)
{
    const std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {
        descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::TEXTURE2D)].Get(),
        descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::TEXTURE2D)].Get()
    };

    const VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool.Get(),
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };

    texture_sets_2d.resize(texture_sets_2d.size() + MAX_FRAMES_IN_FLIGHT);

    const auto result = vkAllocateDescriptorSets(device.Get(), &alloc_info, &texture_sets_2d[texture_sets_2d.size() - MAX_FRAMES_IN_FLIGHT]);
    if (result != VK_SUCCESS) ThrowError("Failed to create descriptor sets.", result);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        const VkDescriptorImageInfo face_info = { sampler.Get(), texture.GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

        const VkWriteDescriptorSet set = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = texture_sets_2d[texture_sets_2d.size() - MAX_FRAMES_IN_FLIGHT + i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &face_info
        };

        vkUpdateDescriptorSets(device.Get(), 1, &set, 0, nullptr);
    }
}

void Context::Impl::AddDescriptorSet3D(const Texture& texture)
{
    const std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts = {
        descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::TEXTURE3D)].Get(),
        descriptor_set_layouts[static_cast<size_t>(GraphicsPipelines::TEXTURE3D)].Get()
    };

    const VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool.Get(),
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };

    texture_sets_3d.resize(texture_sets_3d.size() + MAX_FRAMES_IN_FLIGHT);

    const auto result = vkAllocateDescriptorSets(device.Get(), &alloc_info, &texture_sets_3d[texture_sets_3d.size() - MAX_FRAMES_IN_FLIGHT]);
    if (result != VK_SUCCESS) ThrowError("Failed to create descriptor sets.", result);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        const VkDescriptorImageInfo face_info = { sampler.Get(), texture.GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        const VkDescriptorBufferInfo buffer_info = {
            .buffer = uniform_buffers[static_cast<size_t>(UniformBuffers::TEXTURE3D) * MAX_FRAMES_IN_FLIGHT + i].GetBuffer(),
            .offset = 0,
            .range = sizeof(CameraView)
        };

        const std::array<VkWriteDescriptorSet, 2> sets = {{
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = texture_sets_3d[texture_sets_3d.size() - MAX_FRAMES_IN_FLIGHT + i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &buffer_info
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = texture_sets_3d[texture_sets_3d.size() - MAX_FRAMES_IN_FLIGHT + i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &face_info
            }
        }};

        vkUpdateDescriptorSets(device.Get(), static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
    }
}

void Context::Impl::SetParentWindow(void* native_handle)
{
    window.SetParentWindow(native_handle);
}

Context::Context() noexcept {}

Context::Context(std::string_view window_title, int screenw, int screenh) :
    impl{ std::make_unique<Context::Impl>(window_title, screenw, screenh) }
{}

Context::Context(std::string_view window_title) :
    impl{ std::make_unique<Context::Impl>(window_title) }
{}

Context::Context(void* native_handle, bool drop) : impl{ std::make_unique<Context::Impl>(native_handle, drop) } {}

Context::~Context() {}

Context::Context(Context&& c) noexcept : impl{ std::move(c.impl) } {}

Context& Context::operator=(Context&& c) noexcept
{
    impl = std::move(c.impl);
    return *this;
}

bool Context::BeginRendering() const
{
    return impl->BeginRendering();
}

void Context::EndRendering() const
{
    impl->EndRendering();
}

void Context::Render2D(size_t texture, Rect dst) const
{
    impl->Render2D(texture, dst);
}

void Context::Render2D(size_t texture, Rect src, Rect dst) const
{
    impl->Render2D(texture, src, dst);
}

void Context::Color2D(Color color, Rect area) const
{
    impl->Color2D(color, area);
}

void Context::Render3D(size_t texture, Cuboid area, const CameraView& camera) const
{
    impl->Render3D(texture, area, camera);
}

void Context::Render3D(size_t texture, const Model& model, const CameraView& camera) const
{
    impl->Render3D(texture, model, camera);
}

void Context::Color3D(Color color, Cuboid area, const CameraView& camera) const
{
    impl->Color3D(color, area, camera);
}

void Context::RenderTextRel(std::string_view text, size_t font_style, Color color, float x, float y, float size, float row_width,
    HorizontalAlignment halign, VerticalAlignment valign) const
{
    impl->RenderTextRel(text, font_style, color, x, y, size, row_width, halign, valign);
}

void Context::RenderTextAbs(std::string_view text, size_t font_style, Color color, float x, float y, float size, float row_width,
    HorizontalAlignment halign, VerticalAlignment valign) const
{
    impl->RenderTextAbs(text, font_style, color, x, y, size, row_width, halign, valign);
}

void Context::LoadTexture(std::string_view path) const
{
    impl->LoadTexture(path);
}

void Context::LoadTextures(std::span<const std::string_view> paths) const
{
    impl->LoadTextures(paths);
}

void Context::LoadAlphabet(std::string_view path) const
{
    impl->LoadAlphabet(path);
}

void Context::LoadAlphabets(std::span<const std::string_view> paths) const
{
    impl->LoadAlphabets(paths);
}

// const Window& GetWindow() const noexcept;
// VkExtent2D GetSwapchainExtent() const noexcept;
bool Context::WindowMinimized() const noexcept
{
    const auto extent = impl->GetSwapchainExtent();

    return extent.height == 0 && extent.width == 0;
}

Rect Context::GetWindowRect() const noexcept
{
    return impl->GetWindowRect();
}

int Context::GetWindowWidth() const noexcept
{
    return impl->GetSwapchainExtent().width;
}

int Context::GetWindowHeight() const noexcept
{
    return impl->GetSwapchainExtent().height;
}

void Context::SetFramebufferResized() const noexcept
{
    impl->SetFramebufferResized();
}

void Context::SetParentWindow(void* native_handle)
{
    impl->SetParentWindow(native_handle);
}
}