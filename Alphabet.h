#ifndef ALPHABET_H
#define ALPHABET_H

#include <string_view>
#include <unordered_map>
#include "vulkan/vulkan.h"
#include "RenderData.h"
#include "Texture.h"
#include "freetype.h"
#include "Buffer.h"
#include "Constants.h"
#include "DescriptorPool.h"
#include "Context.h"

namespace VKKit {
class FreeType;
class Device;
class CommandPool;
class GraphicsPipeline;
class CommandBuffer;
class DescriptorSetLayout;
class Sampler;

// enum class HorizontalAlignment { LEFT, CENTER, RIGHT };
// enum class VerticalAlignment { TOP, CENTER, BOTTOM };

class Alphabet {
public:
    Alphabet() noexcept;

    Alphabet(const FreeType& ft, std::string_view font_path, VkPhysicalDevice physical_device, const Device& device,
        const CommandPool& pool, const DescriptorSetLayout& layout, const Sampler& sampler, std::span<const Buffer> projection_uniforms,
        VkSampleCountFlagBits samples);

    void RenderTextRel(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, const CommandBuffer& command_buffer,
        const GraphicsPipeline& pipeline, const DescriptorSetLayout& layout, const Sampler& sampler, std::span<const Buffer> projection_uniforms, uint32_t current_frame, std::string_view text,
        Color color, float font_size, float x, float y, VkExtent2D swapchain_extent, HorizontalAlignment halign, VerticalAlignment valign,
        float row_width = std::numeric_limits<float>::max());

    void RenderTextAbs(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, const CommandBuffer& command_buffer,
        const GraphicsPipeline& pipeline, const DescriptorSetLayout& layout, const Sampler& sampler, std::span<const Buffer> projection_uniforms, uint32_t current_frame, std::string_view text,
        Color color, float font_size, float x, float y, HorizontalAlignment halign, VerticalAlignment valign,
        float row_width = std::numeric_limits<float>::max());

    void ClearBuffers(uint32_t current_frame);

private:
    // Information about a glyph
    struct Glyph {
        glm::vec<2, unsigned> size;
        glm::vec<2, int> bearing;
        FT_Pos advance;
    };

    VkDevice device;
    std::unordered_map<FT_ULong, Texture> bitmaps;
    DescriptorPool descriptor_pool;
    std::array<std::vector<VkDescriptorSet>, MAX_FRAMES_IN_FLIGHT> bitmap_descriptors;
    
    std::unordered_map<FT_ULong, Glyph> glyphs;
    std::array<std::vector<Buffer>, MAX_FRAMES_IN_FLIGHT> vertex_buffers, index_buffers;
    std::array<std::vector<Buffer>, MAX_FRAMES_IN_FLIGHT> vertex_staging_buffers, index_staging_buffers;
    std::array<size_t, MAX_FRAMES_IN_FLIGHT> current_buffer_positions;
    std::array<std::vector<Buffer>, MAX_FRAMES_IN_FLIGHT> color_uniforms;
    std::array<std::vector<void*>, MAX_FRAMES_IN_FLIGHT> color_uniforms_mapped;

    // Text rendering

    size_t RenderTextRow(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, const CommandBuffer& command_buffer,
        const GraphicsPipeline& pipeline, const DescriptorSetLayout& layout, const Sampler& sampler, std::span<const Buffer> projection_uniforms,
        uint32_t current_frame, std::string_view row, float font_size, float x, float y, float row_width);

    void RenderWord(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, const CommandBuffer& command_buffer,
        const GraphicsPipeline& pipeline, const DescriptorSetLayout& layout, const Sampler& sampler, std::span<const Buffer> projection_uniforms,
        uint32_t current_frame, std::string_view word, float font_size, float x, float y);

    std::pair<float, float> RenderWordMultiline(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool,
        const CommandBuffer& command_buffer, const GraphicsPipeline& pipeline, const DescriptorSetLayout& layout, const Sampler& sampler,
        std::span<const Buffer> projection_uniforms, uint32_t current_frame, std::string_view word, float font_size, float x, float y,
        float row_width, HorizontalAlignment halign, VerticalAlignment valign);

    void RenderChar(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, const CommandBuffer& command_buffer,
        const GraphicsPipeline& pipeline, const DescriptorSetLayout& layout, const Sampler& sampler, std::span<const Buffer> projection_uniforms,
        uint32_t current_frame, FT_ULong ch, float font_size, float x, float y);

    void RenderCharOpt(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, const CommandBuffer& command_buffer,
        const GraphicsPipeline& pipeline, const DescriptorSetLayout& layout, const Sampler& sampler, std::span<const Buffer> projection_uniforms,
        uint32_t current_frame, FT_ULong ch, float font_size, float x, float y);

    // Text manipulation
    
    float GetWordWidth(std::string_view word, float font_size);
    std::string_view GetRowSubword(std::string_view word, float font_size, float row_width);
    float GetRowSubwordWidth(std::string_view word, float font_size, float row_width);
    size_t GetRowCount(std::string_view text, float font_size, float row_width);
    float GetRowWidth(std::string_view text, float font_size, float row_width);

    // Construction helper functions

    void CreateDescriptorPool(const Device& device, const DescriptorSetLayout& layout, size_t total_glyphs);
};
}

#endif