#include <numeric>
#include "Alphabet.h"
#include "InitLibs.h"
#include "Device.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "GraphicsPipeline.h"
#include "DescriptorSetLayout.h"
#include "VkResultString.h"
#include "Sampler.h"

namespace VKKit {
static constexpr std::array<unsigned, 6> indices = {
    0, 1, 2, 2, 3, 0
};

static void ThrowFTError(std::string_view error_string, FT_Error code)
{
    char error[256];
    snprintf(error, sizeof(error), "%s. Error: %s", error_string.data(), FT_Error_String(code));
    throw std::runtime_error(error);
}

// Gets the next word in the character sequence
static constexpr std::string_view GetWord(std::string_view text)
{
    for (size_t i = 0; i < text.size(); ++i) {
        assert(text[i] != EOF);
        if (isspace(static_cast<unsigned char>(text[i])))
            return { text.data(), i };
    }

    return text;
}

static VkDescriptorSet CreateGlyphDescriptor(const Device& device, const DescriptorPool& pool, const DescriptorSetLayout& layout,
    const Sampler& sampler, const Texture& texture, const Buffer& color_uniform_buffer, const Buffer& projection_uniform_buffer)
{
    const std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {
        layout.Get(),
        layout.Get()
    };

    const VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool.Get(),
        .descriptorSetCount = 1,
        .pSetLayouts = layouts.data(),
    };

    VkDescriptorSet set{};

    const auto result = vkAllocateDescriptorSets(device.Get(), &alloc_info, &set);
    if (result != VK_SUCCESS) ThrowError("Failed to create descriptor sets.", result);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        const VkDescriptorImageInfo face_info = { sampler.Get(), texture.GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        const VkDescriptorBufferInfo color_buffer_info = { color_uniform_buffer.GetBuffer(), 0, sizeof(Color) };
        const VkDescriptorBufferInfo projection_buffer_info = { projection_uniform_buffer.GetBuffer(), 0, sizeof(glm::mat4) };

        const std::array<VkWriteDescriptorSet, 3> write_sets = {{
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &face_info
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set,
                .dstBinding = 1,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &color_buffer_info
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = set,
                .dstBinding = 2,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &projection_buffer_info
            }
        }};

        vkUpdateDescriptorSets(device.Get(), static_cast<uint32_t>(write_sets.size()), write_sets.data(), 0, nullptr);
    }

    return set;
}

namespace {
class Face {
public:
    Face() noexcept : face{ nullptr } {}

    Face(const FreeType& ft, std::string_view font_path)
    {
        const auto result = FT_New_Face(ft.Get(), font_path.data(), 0, &face);

        if (result) ThrowFTError("Failed to create FreeType face", result);
    }

    ~Face()
    {
        FT_Done_Face(face);
    }

    Face(const Face&) = delete;
    Face& operator=(const Face&) = delete;
    Face(Face&& f) noexcept :
        face{ f.face }
    {
        f.face = nullptr;
    }
    Face& operator=(Face&& f) noexcept
    {
        FT_Done_Face(face);
        face = f.face;
        f.face = nullptr;
        return *this;
    }

    void SetPixelSizes(FT_UInt width, FT_UInt height) const noexcept
    {
        FT_Set_Pixel_Sizes(face, width, height);
    }

    void LoadGlyph(FT_ULong glyph) const
    {
        const auto result = FT_Load_Char(face, glyph, FT_LOAD_RENDER);

        if (result) ThrowFTError("Failed to load char", result);
    }

    FT_Face Get() const noexcept { return face; }
    FT_GlyphSlot GetGlyph() const noexcept { return face->glyph; }

private:
    FT_Face face;
};
}

static size_t GetFontGlyphCount(const Face& face)
{
    FT_ULong charcode;
    FT_UInt gindex;

    size_t count = 0;

    charcode = FT_Get_First_Char(face.Get(), &gindex);

    while (gindex != 0) {
        ++count;
        charcode = FT_Get_Next_Char(face.Get(), charcode, &gindex);
    }
    
    return count;
}

Alphabet::Alphabet() noexcept :
    device{ nullptr },
    color_uniforms_mapped{},
    current_buffer_positions{}
{}

Alphabet::Alphabet(const FreeType& ft, std::string_view font_path, VkPhysicalDevice physical_device, const Device& device,
    const CommandPool& pool, const DescriptorSetLayout& layout, const Sampler& sampler, std::span<const Buffer> projection_uniforms,
    VkSampleCountFlagBits samples) :
    device{ device.Get() },
    current_buffer_positions{}
{
    (void)samples; // Find a use for this or remove it
    (void)projection_uniforms; // Find a use for this or remove it
    (void)sampler; // Find a use for this or remove it

    const Face face(ft, font_path);
    face.SetPixelSizes(0, static_cast<int>(BASE_FONT_HEIGHT));

    const size_t total_glyphs = GetFontGlyphCount(face);

    glyphs.reserve(total_glyphs);
    bitmaps.reserve(total_glyphs);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        color_uniforms[i].resize(200 * MAX_FRAMES_IN_FLIGHT); // Quick hack to get uniform buffers working, should be fixed eventually
        color_uniforms_mapped[i].resize(200 * MAX_FRAMES_IN_FLIGHT);
        
        for (size_t j = 0; j < color_uniforms.size(); ++j) {
            color_uniforms[i][j] = Buffer(physical_device, device.Get(), sizeof(Color), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_SHARING_MODE_EXCLUSIVE);
        }
    }

    CreateDescriptorPool(device, layout, total_glyphs);

    for (FT_ULong charcode = FIRST_PRINTABLE_ASCII; charcode <= LAST_PRINTABLE_ASCII; ++charcode) {
        if (FT_Get_Char_Index(face.Get(), charcode) == 0) continue;
        face.LoadGlyph(charcode);

        const unsigned width = face.GetGlyph()->bitmap.width;
        const unsigned rows = face.GetGlyph()->bitmap.rows;
        const FT_Int bearingx = face.GetGlyph()->bitmap_left;
        const FT_Int bearingy = face.GetGlyph()->bitmap_top;
        const FT_Pos advancex = face.GetGlyph()->advance.x;
        const unsigned char* bitmap = face.GetGlyph()->bitmap.buffer;
        const size_t bitmap_size = width * rows;

        glyphs.insert({ charcode, Glyph { { width, rows }, { bearingx, bearingy }, advancex }});

        if (bitmap_size != 0) {
            bitmaps.insert({ charcode, Texture(physical_device, device, pool, VK_FORMAT_R8_SRGB, width, rows, { bitmap, bitmap_size },
                VK_IMAGE_TILING_OPTIMAL, VK_SAMPLE_COUNT_1_BIT, 1) });
        }
    }

    //CreateDescriptors(device, layout, sampler, projection_uniforms);
}

void Alphabet::RenderTextRel(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, const CommandBuffer& command_buffer,
    const GraphicsPipeline& pipeline, const DescriptorSetLayout& layout, const Sampler& sampler, std::span<const Buffer> projection_uniforms,
    uint32_t current_frame, std::string_view text, Color color, float font_size, float x, float y, VkExtent2D swapchain_extent, HorizontalAlignment halign,
    VerticalAlignment valign, float row_width)
{
    vertex_buffers[current_frame].reserve(text.size());
    index_buffers[current_frame].reserve(text.size());

    color_uniforms[current_frame].emplace_back(physical_device, device.Get(), sizeof(Color), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_SHARING_MODE_EXCLUSIVE);
    // color_uniforms_mapped[current_frame].emplace_back(color_uniforms[current_frame].back().GetMemory());
    color_uniforms_mapped[current_frame].push_back(nullptr);
    vkMapMemory(device.Get(), color_uniforms[current_frame].back().GetMemory(), 0, sizeof(Color), 0, &color_uniforms_mapped[current_frame].back());

    memcpy(color_uniforms_mapped[current_frame].back(), &color, sizeof(Color));

    vkCmdBindPipeline(command_buffer.GetBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetPipeline());

    // The default resolution for relative rendering is 1920x1080, which is when textures are rendered at their normal size.
    // If the screen is a different size than 1920x1080, textures will be scaled up/down
    const float xscale = static_cast<float>(swapchain_extent.width) / DEFAULT_SCREEN_WIDTH;
    const float yscale = static_cast<float>(swapchain_extent.height) / DEFAULT_SCREEN_HEIGHT;

    x *= xscale;
    y *= yscale;
    font_size *= yscale;
    row_width *= xscale;

    const size_t rows = GetRowCount(text, font_size, row_width);
    const float text_area_height = font_size * rows * yscale;

    switch (valign) {
    case VerticalAlignment::TOP: break;
    case VerticalAlignment::CENTER: y -= text_area_height / 2.0f; break;
    case VerticalAlignment::BOTTOM: y -= text_area_height; break;
    };

    float width_indent{};
    switch (halign) {
    case HorizontalAlignment::LEFT: width_indent = 0.0f; break;
    case HorizontalAlignment::CENTER: width_indent = 0.5f; break;
    case HorizontalAlignment::RIGHT: width_indent = 1.0f; break;
    };

    size_t index = 0;
    float xoffset = 0.0f;
    float yoffset = 0.0f;
    while (index < text.size()) {
        const float width = GetRowWidth({ text.data() + index, text.size() - index }, font_size, row_width - xoffset);
        const float realx = x - width * width_indent;
        xoffset = 0.0f;

        if (width != 0.0f) {
            index += RenderTextRow(physical_device, device, pool, command_buffer, pipeline, layout, sampler, projection_uniforms, current_frame,
                { text.data() + index, text.size() - index }, font_size, realx, y + yoffset, row_width) + 1;

            yoffset -= font_size;
        }
        else {
            // If the row width is 0.0f, we need to render a multiline word
            const auto word = GetWord({ text.data() + index, text.size() - index });
            if (word.size() == 0) { // whitespace
                yoffset += font_size;
                ++index;
                continue;
            }

            const auto displacement = RenderWordMultiline(physical_device, device, pool, command_buffer, pipeline, layout, sampler, projection_uniforms,
                current_frame, word, font_size, realx, y, row_width, halign, valign);

            xoffset = displacement.first;
            yoffset += displacement.second;
            index += word.size();
        }
    }
}

void Alphabet::RenderTextAbs(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, const CommandBuffer& command_buffer,
    const GraphicsPipeline& pipeline, const DescriptorSetLayout& layout, const Sampler& sampler, std::span<const Buffer> projection_uniforms,
    uint32_t current_frame, std::string_view text, Color color, float font_size, float x, float y, HorizontalAlignment halign, VerticalAlignment valign,
    float row_width)
{
    RenderTextRel(physical_device, device, pool, command_buffer, pipeline, layout, sampler, projection_uniforms, current_frame, text, color, font_size,
        x, y, DEFAULT_EXTENT, halign, valign, row_width);
}

void Alphabet::ClearBuffers(uint32_t current_frame)
{
    /*vertex_buffers[current_frame].clear();
    index_buffers[current_frame].clear();*/
    current_buffer_positions[current_frame] = 0;
    color_uniforms[current_frame].clear();
    color_uniforms_mapped[current_frame].clear();
    if (bitmap_descriptors[current_frame].size() > 0)
        vkFreeDescriptorSets(device, descriptor_pool.Get(), static_cast<uint32_t>(bitmap_descriptors[current_frame].size()), bitmap_descriptors[current_frame].data());
    bitmap_descriptors[current_frame].clear();
}

size_t Alphabet::RenderTextRow(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, const CommandBuffer& command_buffer,
    const GraphicsPipeline& pipeline, const DescriptorSetLayout& layout, const Sampler& sampler, std::span<const Buffer> projection_uniforms,
    uint32_t current_frame, std::string_view row, float font_size, float x, float y, float row_width)
{
    size_t chars_rendered;

    float xoffset = 0.0f;
    for (chars_rendered = 0; chars_rendered < row.size(); ++chars_rendered) {
        if (row[chars_rendered] == '\n') break;

        const FT_ULong c = static_cast<FT_ULong>(row[chars_rendered]);

        if (c != FIRST_PRINTABLE_ASCII) {
            const auto word = GetWord({ row.data() + chars_rendered, row.size() - chars_rendered});
            const float distance_to_row_end = row_width - xoffset;
            const float word_width = GetWordWidth(word, font_size);

            if (word_width <= distance_to_row_end) {
                RenderWord(physical_device, device, pool, command_buffer, pipeline, layout, sampler, projection_uniforms, current_frame, word, font_size,
                    x + xoffset, y);
                chars_rendered += word.size() - 1;
                xoffset += word_width;
            }
            else {
                if (chars_rendered == 0) {
                    // Big multiline word found at the beginning of the row, render only the part of it that fits inside a single row
                    const auto subword = GetRowSubword(row, font_size, row_width);
                    RenderWord(physical_device, device, pool, command_buffer, pipeline, layout, sampler, projection_uniforms, current_frame, subword,
                        font_size, x, y);
                    return subword.size() - 1;
                }
                else {
                    --chars_rendered;
                    break;
                }
            }
        }
        else {
            const float advance = static_cast<float>(glyphs.at(c).advance / 64) * font_size / BASE_FONT_HEIGHT;

            if (xoffset < row_width - advance) {
                xoffset += advance;
            }
            else {
                break;
            }
        }
    }

    return chars_rendered;
}

void Alphabet::RenderWord(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, const CommandBuffer& command_buffer,
    const GraphicsPipeline& pipeline, const DescriptorSetLayout& layout, const Sampler& sampler, std::span<const Buffer> projection_uniforms,
    uint32_t current_frame, std::string_view word, float font_size, float x, float y)
{
    float xoffset = 0.0f;
    for (const auto ch : word) {
        const FT_ULong c = static_cast<FT_ULong>(ch);

        /*RenderChar(physical_device, device, pool, command_buffer, pipeline, layout, sampler, projection_uniforms, current_frame, c, font_size,
            x + xoffset, y);*/
        RenderCharOpt(physical_device, device, pool, command_buffer, pipeline, layout, sampler, projection_uniforms, current_frame, c, font_size,
            x + xoffset, y);

        const float advance = static_cast<float>(glyphs.at(c).advance / 64) * font_size / BASE_FONT_HEIGHT;
        xoffset += advance;
    }
}

// Renders a word over multiple lines. Returns a pair of two floats, where the first is the horizontal offset after rendering, and the second is the
// vertical displacement (how much the text has gone up/down) that happened after the rendering.
std::pair<float, float> Alphabet::RenderWordMultiline(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool,
    const CommandBuffer& command_buffer, const GraphicsPipeline& pipeline, const DescriptorSetLayout& layout, const Sampler& sampler,
    std::span<const Buffer> projection_uniforms, uint32_t current_frame, std::string_view word, float font_size, float x, float y, float row_width,
    HorizontalAlignment halign, VerticalAlignment valign)
{
    (void)halign; // Find a use for this or remove it
    (void)valign; // Find a use for this or remove it

    float xoffset = 0.0f;
    float yoffset = 0.0f;

    for (const auto ch : word) {
        const FT_ULong c = static_cast<FT_ULong>(ch);

        /*RenderChar(physical_device, device, pool, command_buffer, pipeline, layout, sampler, projection_uniforms, current_frame, c, font_size,
            x + xoffset, y + yoffset);*/
        RenderCharOpt(physical_device, device, pool, command_buffer, pipeline, layout, sampler, projection_uniforms, current_frame, c, font_size,
            x + xoffset, y + yoffset);

        const float advance = static_cast<float>(glyphs.at(c).advance / 64) * font_size / BASE_FONT_HEIGHT;

        if (xoffset < row_width - advance) {
            xoffset += advance;
        }
        else {
            xoffset = 0.0f;
            yoffset -= font_size;
        }
    }

    return { xoffset, yoffset };
}

void Alphabet::RenderChar(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, const CommandBuffer& command_buffer,
    const GraphicsPipeline& pipeline, const DescriptorSetLayout& layout, const Sampler& sampler, std::span<const Buffer> projection_uniforms, uint32_t current_frame, FT_ULong ch, float font_size,
    float x, float y)
{
    const Glyph glyph = glyphs.at(ch);

    // Normalized coordinates
    const float xpos = x + glyph.bearing.x * font_size / BASE_FONT_HEIGHT;
    const float ypos = y - (glyph.size.y - glyph.bearing.y) * font_size / BASE_FONT_HEIGHT;
    const float width = static_cast<float>(glyph.size.x) * font_size / BASE_FONT_HEIGHT;
    const float height = static_cast<float>(glyph.size.y) * font_size / BASE_FONT_HEIGHT;

    const std::array<float, 16> vertices = {
        xpos,           ypos,          0.0f, 1.0f, // Top left
        xpos + width,   ypos,          1.0f, 1.0f, // Top right
        xpos + width,   ypos + height, 1.0f, 0.0f, // Bottom right
        xpos,           ypos + height, 0.0f, 0.0f  // Bottom left
    };

    vertex_buffers[current_frame].push_back(Buffer::CreateVertexBuffer(physical_device, device, pool, vertices));
    index_buffers[current_frame].push_back(Buffer::CreateIndexBuffer(physical_device, device, pool, indices));

    // Create a new descriptor for every glyph that is rendered and push it back into the array of descriptors.
    bitmap_descriptors[current_frame].push_back(CreateGlyphDescriptor(device, descriptor_pool, layout, sampler, bitmaps.at(ch), color_uniforms[current_frame].back(),
        projection_uniforms[current_frame]));

    const auto buf = vertex_buffers[current_frame].back().GetBuffer();
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(command_buffer.GetBuffer(), 0, 1, &buf, &offset);
    vkCmdBindIndexBuffer(command_buffer.GetBuffer(), index_buffers[current_frame].back().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    
    vkCmdBindDescriptorSets(command_buffer.GetBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetLayout(), 0, 1, &bitmap_descriptors[current_frame].back(),
        0, nullptr);
    vkCmdDrawIndexed(command_buffer.GetBuffer(), static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

// Fix this function: glyphs are rendered with a very tiny font size
void Alphabet::RenderCharOpt(VkPhysicalDevice physical_device, const Device& device, const CommandPool& pool, const CommandBuffer& command_buffer,
    const GraphicsPipeline& pipeline, const DescriptorSetLayout& layout, const Sampler& sampler, std::span<const Buffer> projection_uniforms, uint32_t current_frame, FT_ULong ch, float font_size,
    float x, float y)
{
    const Glyph glyph = glyphs.at(ch);

    // Normalized coordinates
    const float xpos = x + glyph.bearing.x * font_size / BASE_FONT_HEIGHT;
    const float ypos = y - (glyph.size.y - glyph.bearing.y) * font_size / BASE_FONT_HEIGHT;
    const float width = static_cast<float>(glyph.size.x) * font_size / BASE_FONT_HEIGHT;
    const float height = static_cast<float>(glyph.size.y) * font_size / BASE_FONT_HEIGHT;

    const std::array<float, 16> vertices = {
        xpos,           ypos,          0.0f, 1.0f, // Top left
        xpos + width,   ypos,          1.0f, 1.0f, // Top right
        xpos + width,   ypos + height, 1.0f, 0.0f, // Bottom right
        xpos,           ypos + height, 0.0f, 0.0f  // Bottom left
    };

    /*vertex_buffers[current_frame].push_back(Buffer::CreateVertexBuffer(physical_device, device, pool, vertices));
    index_buffers[current_frame].push_back(Buffer::CreateIndexBuffer(physical_device, device, pool, indices));*/
    auto& bufpos = current_buffer_positions[current_frame];
    auto& vbufferarr = vertex_buffers[current_frame];
    auto& vbufferstaging = vertex_staging_buffers[current_frame];
    auto& ibufferarr = index_buffers[current_frame];
    auto& ibufferstaging = index_staging_buffers[current_frame];

    if (bufpos >= vbufferarr.size()) {
        vbufferarr.push_back(Buffer::CreateVertexBuffer(physical_device, device, pool, vertices));
        vbufferstaging.push_back(CreateStagingBuffer(physical_device, device.Get(), sizeof(vertices)));
        ibufferarr.push_back(Buffer::CreateIndexBuffer(physical_device, device, pool, indices));
        ibufferstaging.push_back(CreateStagingBuffer(physical_device, device.Get(), sizeof(indices)));
    }
    else {
        vbufferarr[bufpos].WriteData(device, pool, vbufferstaging[bufpos], vertices.data(), sizeof(vertices));
        ibufferarr[bufpos].WriteData(device, pool, ibufferstaging[bufpos], indices.data(), sizeof(indices));
    }

    // Create a new descriptor for every glyph that is rendered and push it back into the array of descriptors.
    bitmap_descriptors[current_frame].push_back(CreateGlyphDescriptor(device, descriptor_pool, layout, sampler, bitmaps.at(ch), color_uniforms[current_frame].back(),
        projection_uniforms[current_frame]));

    //const auto buf = vertex_buffers[current_frame].back().GetBuffer();
    const auto buf = vbufferarr[bufpos].GetBuffer();
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(command_buffer.GetBuffer(), 0, 1, &buf, &offset);
    //vkCmdBindIndexBuffer(command_buffer.GetBuffer(), index_buffers[current_frame].back().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindIndexBuffer(command_buffer.GetBuffer(), ibufferarr[bufpos].GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(command_buffer.GetBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetLayout(), 0, 1, &bitmap_descriptors[current_frame].back(),
        0, nullptr);
    vkCmdDrawIndexed(command_buffer.GetBuffer(), static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    ++bufpos;
}

float Alphabet::GetWordWidth(std::string_view word, float font_size)
{
    float width = 0.0f;

    for (const auto ch : word) {
        const FT_ULong c = static_cast<FT_ULong>(ch);

        width += static_cast<float>(glyphs.at(c).advance / 64) * font_size / BASE_FONT_HEIGHT;
    }

    return width;
}

// Get the substring of a long word that fits inside a single row
std::string_view Alphabet::GetRowSubword(std::string_view word, float font_size, float row_width)
{
    float width = 0.0f;
    for (size_t i = 0; i < word.size(); ++i) {
        const FT_ULong c = static_cast<FT_ULong>(word[i]);

        const auto advance = static_cast<float>(glyphs.at(c).advance / 64) * font_size / BASE_FONT_HEIGHT;

        if (width < row_width - advance)
            width += advance;
        else
            return { word.data(), i };
    }

    return word;
}

// Get the width of a substring of a long word that fits inside a single row
float Alphabet::GetRowSubwordWidth(std::string_view word, float font_size, float row_width)
{
    return GetWordWidth(GetRowSubword(word, font_size, row_width), font_size);
}

size_t Alphabet::GetRowCount(std::string_view text, float font_size, float row_width)
{
    size_t rows = 1; // Every text will have at least 1 row
    float xoffset = 0.0f;
    for (const auto ch : text) {
        if (ch == '\n') {
            ++rows;
            continue;
        }

        const FT_ULong c = static_cast<FT_ULong>(ch);

        const float advance = static_cast<float>(glyphs.at(c).advance / 64) * font_size / BASE_FONT_HEIGHT;

        if (xoffset < row_width - advance) {
            xoffset += advance;
        }
        else {
            xoffset = 0.0f;
            ++rows;
        }
    }

    return rows;
}

float Alphabet::GetRowWidth(std::string_view text, float font_size, float row_width)
{
    float width = 0.0f;

    for (size_t i = 0; i < text.size(); ++i) {
        const FT_ULong c = static_cast<FT_ULong>(text[i]);

        if (c != FIRST_PRINTABLE_ASCII) {
            const auto word = GetWord({ text.data() + i, text.size() - i });
            const auto word_width = GetWordWidth(word, font_size);

            if (word_width < row_width - width) {
                width += word_width;
                i += word.size() - 1;
            }
            else {
                if (i == 0) {
                    // A big word starts at the beginning of the row, return the width of the part of it that fits in the row
                    return GetRowSubwordWidth(GetRowSubword(word, font_size, row_width), font_size, row_width);
                }
                else {
                    // It's not a big multiline word at the beginning of the row, return the width so far
                    return width;
                }
            }
        }
        else {
            const float advance = static_cast<float>(glyphs.at(c).advance / 64) * font_size / BASE_FONT_HEIGHT;

            if (width < row_width - advance) {
                width += advance;
            }
            else {
                return width;
            }
        }
    }

    return width;
}

void Alphabet::CreateDescriptorPool(const Device& device, const DescriptorSetLayout& layout, size_t total_glyphs)
{
    (void)layout; // Find a use for this or remove it
    const std::array<VkDescriptorPoolSize, 1> pool_sizes = {{
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = static_cast<uint32_t>(total_glyphs * MAX_FRAMES_IN_FLIGHT) * 100 // TO DO: Pick a more reasonable amount of descriptors
        }
    }};

    this->descriptor_pool = DescriptorPool(device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, pool_sizes, MAX_FRAMES_IN_FLIGHT);
}

// void Alphabet::CreateDescriptors(const Device& device, const DescriptorSetLayout& layout, const Sampler& sampler, std::span<const Buffer> projection_uniforms)
// {
//     AllocateDescriptors(device, layout, sampler);
    
//     for (const auto& bitmap : bitmaps) {
//         const auto& descriptors = bitmap_descriptors.at(bitmap.first);

//         const VkDescriptorImageInfo image_info = { sampler.Get(), bitmap.second.GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

//         for (size_t c = 0; c < color_uniforms.size(); ++c) {
//             for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
//                 const VkDescriptorBufferInfo color_buffer_info = { color_uniforms[i][c].GetBuffer(), 0, sizeof(Color) };
//                 const VkDescriptorBufferInfo projection_buffer_info = { projection_uniforms[i].GetBuffer(), 0, sizeof(glm::mat4) };

//                 /* Idea for solution: Every time a text rendering call is made, generate new descriptor sets with the desired color */
//                 const std::array<VkWriteDescriptorSet, 3> descriptor_writes = {{
//                     {
//                         .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//                         .dstSet = descriptors[i],
//                         .dstBinding = 0,
//                         .descriptorCount = 1,
//                         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//                         .pImageInfo = &image_info
//                     },
//                     {
//                         .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//                         .dstSet = descriptors[i],
//                         .dstBinding = 1,
//                         .descriptorCount = 1,
//                         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//                         .pBufferInfo = &color_buffer_info
//                     },
//                     {
//                         .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//                         .dstSet = descriptors[i],
//                         .dstBinding = 2,
//                         .descriptorCount = 1,
//                         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//                         .pBufferInfo = &projection_buffer_info
//                     }
//                 }};

//                 vkUpdateDescriptorSets(device.Get(), descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
//             }
//         }
//     }
// }

// void Alphabet::AllocateDescriptors(const Device& device, const DescriptorSetLayout& layout, const Sampler& sampler)
// {
//     const std::vector<VkDescriptorSetLayout> layouts(bitmaps.size() * MAX_FRAMES_IN_FLIGHT, layout.Get());

//     const VkDescriptorSetAllocateInfo alloc_info = {
//         .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
//         .descriptorPool = descriptor_pool.Get(),
//         .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
//         .pSetLayouts = layouts.data()
//     };

//     bitmap_descriptors.reserve(bitmaps.size() * MAX_FRAMES_IN_FLIGHT);

//     std::vector<VkDescriptorSet> descriptors_temp(bitmaps.size() * MAX_FRAMES_IN_FLIGHT);
//     const auto alloc_result = vkAllocateDescriptorSets(device.Get(), &alloc_info, descriptors_temp.data());
//     if (alloc_result != VK_SUCCESS) ThrowError("Failed to allocate descriptors for alphabet.", alloc_result);

//     size_t current_descriptor_pair = 0;
//     for (const auto& bitmap : bitmaps) {
//         std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptors_to_add;
//         for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
//             descriptors_to_add[i] = descriptors_temp[i + current_descriptor_pair];
//         current_descriptor_pair += MAX_FRAMES_IN_FLIGHT;

//         bitmap_descriptors.insert({ bitmap.first, descriptors_to_add });
//     }
// }
}