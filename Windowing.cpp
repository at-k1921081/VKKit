#include <stdexcept>
#include <string>
#include "Windowing.h"
#include "SDL.h"
#include "SString.h"
#include "SDL_syswm.h"

using namespace std::literals;

namespace VKKit {
Window::Window(std::string_view title, int w, int h) :
    window{ SDL_CreateWindow(title.data(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE) }
{
    if (!window) {
        char error[256];
        snprintf(error, sizeof(error), "Failed to create window. Error: %s", SDL_GetError());
        throw std::runtime_error(error);
    }
    SDL_SetWindowResizable(window, SDL_TRUE);
}

Window::Window(std::string_view title) :
    window{ SDL_CreateWindow(title.data(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN) }
{
    if (!window) {
        char error[256];
        snprintf(error, sizeof(error), "Failed to create window. Error: %s", SDL_GetError());
        throw std::runtime_error(error);
    }
}

#if defined _WIN32 || defined __linux__
Window::Window(void* native_handle, bool drop)
{
    SDL_SetHint(SDL_HINT_VIDEO_FOREIGN_WINDOW_VULKAN, "1");

    window = SDL_CreateWindowFrom(native_handle);

    if (!window) {
        char error[256];
        snprintf(error, sizeof(error), "Failed to create window. Error: %s", SDL_GetError());
        throw std::runtime_error(error);
    }

    this->drop = drop;
}
#endif

Window::~Window()
{
    if (drop) SDL_DestroyWindow(window);
}

Window::Window(Window&& win) noexcept :
    window{ win.window }
{
    win.window = nullptr;
}

Window& Window::operator=(Window&& win) noexcept
{
    SDL_DestroyWindow(window);
    window = win.window;
    win.window = nullptr;
    return *this;
}

bool Window::IsMinimised() const noexcept
{
    int flags = SDL_GetWindowFlags(window);
    int minimised = (flags & SDL_WINDOW_MINIMIZED);
    return minimised == 1;
}

bool Window::IsMaximised() const noexcept
{
    return (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) == 1;
}

bool Window::IsFullscreen() const noexcept
{
    return (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) == 1;
}

#ifdef _WIN32
void Window::SetParentWindow(void* native_handle)
{
    HWND hwnd = reinterpret_cast<HWND>(native_handle);
    SDL_SysWMinfo info;
    SDL_GetWindowWMInfo(Get(), &info);
    SetParent(info.info.win.window, hwnd);
}
#endif

}