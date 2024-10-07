#ifndef VKKITCONSTANTS_H
#define VKKITCONSTANTS_H

#include <array>
#include "RenderData.h"
#include "vulkan/vulkan.h"
#include "freetype.h"

namespace VKKit {
constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

constexpr Color RED = { 1.0f, 0.0f, 0.0f, 1.0f };
constexpr Color GREEN = { 0.0f, 1.0f, 0.0f, 1.0f };
constexpr Color BLUE = { 0.0f, 0.0f, 1.0f, 1.0f };
constexpr Color YELLOW = { 1.0f, 1.0f, 0.0f, 1.0f };
constexpr Color WHITE = { 1.0f, 1.0f, 1.0f, 1.0f };
constexpr Color BLACK = { 0.0f, 0.0f, 0.0f, 1.0f };

constexpr FT_ULong FIRST_PRINTABLE_ASCII = 32;
constexpr FT_ULong LAST_PRINTABLE_ASCII = 126;
// static constexpr FT_ULong TOTAL_ASCII_GLYPHS = 95;

constexpr float BASE_FONT_HEIGHT = 64.0f;

// When rendering text with relative coordinates, a base resolution of 1080p will be assumed (and scaled to other resolutions)
constexpr float DEFAULT_SCREEN_WIDTH = 1920.0f, DEFAULT_SCREEN_HEIGHT = 1080.0f;
constexpr float DEFSCREENW = 1920.0f, DEFSCREENH = 1080.0f;
constexpr VkExtent2D DEFAULT_EXTENT = { 1920, 1080 };

constexpr std::array<const char*, 1> DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef _WIN32
#define VKKIT_DIRECTORY "C:/Users/Alex/Desktop/Development/ProjectsLibraries/VKKit"
#elif defined __linux__
#define VKKIT_DIRECTORY "/home/alex/Desktop/Development/ProjectsLibraries/VKKit"
#endif
}

#endif