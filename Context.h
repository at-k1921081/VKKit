#ifndef VKKITCONTEXT_H
#define VKKITCONTEXT_H

#include <string_view>
#include <span>
#include <limits>
#include <memory>

#include "RenderData.h"

namespace VKKit {
class Model;

// How to horizontally align text that is rendered on the screen
enum class HorizontalAlignment { LEFT, CENTER, RIGHT };

// How to vertically align text that is rendered on the screen
enum class VerticalAlignment { TOP, CENTER, BOTTOM };

// A Vulkan rendering context that renders using the Vulkan API
class Context {
public:
    Context() noexcept;

    /**
     * @brief Creates a new window with a Vulkan rendering context added to it.
     * @param window_title The title of the window
     * @param screenw The width of the window
     * @param screenh The height of the window
     * 
     * @exception std::runtime_error with error information on failure
     */
    Context(std::string_view window_title, int screenw, int screenh);

    /**
     * @brief Creates a new fullscreen window with a Vulkan rendering context added to it.
     * @param window_title The title of the window
     * 
     * @exception std::runtime_error with error information on failure
     */
    Context(std::string_view window_title);

    /**
     * @brief Create a Vulkan rendering context on a native window supplied.
     * @param native_handle A native window handle (HWND on Windows, xcb_window_t on Linux, etc.)
     * @param drop Should this context be responsible for closing the window when it gets out of scope?
     * 
     * @exception std::runtime_error with error information on failure
     */
    Context(void* native_handle, bool drop);

    ~Context();

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&& c) noexcept;
    Context& operator=(Context&& c) noexcept;

    /**
     * @brief Begin rendering the current frame.
     * @return true if rendering has begun successfully, false otherwise
     */
    bool BeginRendering() const;

    // End rendering the current frame and present the results on the screen.
    void EndRendering() const;

    /**
     * @brief Render a 2D image on the screen
     * @param texture The index of the texture. This is the index at which the image is stored in the context's internal array.
     * @param dst The destination on the screen on which to render (in normalized Vulkan coordinates).
     */
    void Render2D(size_t texture, Rect dst) const;

    /**
     * @brief Render part of a 2D image on the screen
     * @param texture The index of the texture. This is the index at which the image is stored in the context's internal array.
     * @param src The part of the texture to render (in normalized Vulkan coordinates).
     * @param dst The destination on the screen on which to render (in normalized Vulkan coordiantes).
     */
    void Render2D(size_t texture, Rect src, Rect dst) const;

    /**
     * @brief Render a rectangle on the screen
     * @param color The color of the rectangle
     * @param area The destination on the screen on which to render (in normalized Vulkan coordinates).
     */
    void Color2D(Color color, Rect area) const;

    /**
     * @brief Render a textured cuboid on the screen
     * @param texture The index of the texture (this is the index at which the texture is found in the context's internal array).
     * @param area The destination on the screen on which to render (in normalized Vulkan coordinates)
     * @param camera The camera view which will be looking at the scene
     */
    void Render3D(size_t texture, Cuboid area, const CameraView& camera) const;

    /**
     * @brief Render a textured model on the screen
     * @param texture The index of the texture (this is the index at which the texture is found in the context's internal array).
     * @param model The model to be used for the rendering
     * @param camera The camera view which will be looking at the scene
     */
    void Render3D(size_t texture, const Model& model, const CameraView& camera) const;

    /**
     * @brief Render a colored cuboid on the screen
     * @param color The color of the cuboid
     * @param area The destination on which to render (in normalized Vulkan coordinates)
     * @param camera The camera view which will look at the scene
     */
    void Color3D(Color color, Cuboid area, const CameraView& camera) const;

    /**
     * @brief Render text on the screen with relative sizing. Text will appear full size on a 1920x1080 display, and will be
     *        scaled up/down if the display is different.
     * @param text The text to render
     * @param font_style The font style. This is the index of the font which is stored in an internal array.
     * @param color The color of the text.
     * @param x The x position of the text on the screen
     * @param y The y position of the text on the screen
     * @param size The size of the text
     * @param row_width The width of a row. This is used to determine how much horizontal space text can take before moving to a new row.
     * @param halign The horizontal alignment of the text (left/center/right)
     * @param valign The vertical alignment of the text (top/center/bottom)
     */
    void RenderTextRel(std::string_view text, size_t font_style, Color color, float x, float y, float size, float row_width = std::numeric_limits<float>::max(),
        HorizontalAlignment halign = HorizontalAlignment::LEFT, VerticalAlignment valign = VerticalAlignment::TOP) const;

    /**
     * @brief Render text on the screen with absolute sizing. The text will have the same size no matter the window size or display resolution.
     * @param text The text to render
     * @param font_style The font style. This is the index of the font which is stored in an internal array.
     * @param color The color of the text.
     * @param x The x position of the text on the screen
     * @param y The y position of the text on the screen
     * @param size The size of the text
     * @param row_width The width of a row. This is used to determine how much horizontal space text can take before moving to a new row.
     * @param halign The horizontal alignment of the text (left/center/right)
     * @param valign The vertical alignment of the text (top/center/bottom)
     */
    void RenderTextAbs(std::string_view text, size_t font_style, Color color, float x, float y, float size, float row_width = std::numeric_limits<float>::max(),
        HorizontalAlignment halign = HorizontalAlignment::LEFT, VerticalAlignment valign = VerticalAlignment::TOP) const;

    /** 
     * @brief Load a texture from path and append to the array of textures.
     * @param path The path to the file of the texture. If empty, appends an empty texture to the array.
     * @exception std::runtime_error with error information on failure
     */
    void LoadTexture(std::string_view path) const;

    /**
     * @brief Loads a number of textures from different file paths.
     * @param paths The array of file paths of the textures. If a file path is empty, an empty texture will be appended to the array.
     * @exception std::runtime_error with error information if creating a texture fails
     */
    void LoadTextures(std::span<const std::string_view> paths) const;

    /**
     * @brief Loads a font for rendering and appends it to the loaded fonts.
     * @param path The path to the font's file
     * @exception std::runtime_error with error information on failure
     */
    void LoadAlphabet(std::string_view path) const;

    /**
     * @brief Loads a list of fonts for rendering and appends them to the loaded fonts.
     * @param paths The array of font file paths
     * @exception std::runtime_error with error information if creating a font fails
     */
    void LoadAlphabets(std::span<const std::string_view> paths) const;

    // const Window& GetWindow() const noexcept;
    // VkExtent2D GetSwapchainExtent() const noexcept;

    // Is the window minimized. Returns true if it is, false otherwise.
    bool WindowMinimized() const noexcept;

    // Gets a rectangle of the window that contains its size and position
    Rect GetWindowRect() const noexcept;

    // Returns the width of the window of the context.
    int GetWindowWidth() const noexcept;

    // Returns the heightof the window of the context.
    int GetWindowHeight() const noexcept;

    // Notifies the context that the framebuffer has been resized. Call this function when resizing the window.
    void SetFramebufferResized() const noexcept;

    void SetParentWindow(void* native_handle);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};
}

#endif