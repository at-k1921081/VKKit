#ifndef WINDOWING_H
#define WINDOWING_H

#include <string_view>

struct SDL_Window;

namespace VKKit {
class Window {
public:
    Window(std::string_view title, int w, int h);
    Window(std::string_view title); // Fullscreen window
#if defined _WIN32 || defined __linux
    Window(void* native_handle, bool drop);
#endif
    ~Window();

    Window(const Window& win) = delete;
    Window& operator=(const Window& win) = delete;
    Window(Window&& win) noexcept;
    Window& operator=(Window&& win) noexcept;

    SDL_Window* Get() const noexcept { return window; }

    bool IsMinimised() const noexcept;
    bool IsMaximised() const noexcept;
    bool IsFullscreen() const noexcept;

    void SetParentWindow(void* native_handle);

private:
    SDL_Window* window;
    bool drop;
};
}

#endif