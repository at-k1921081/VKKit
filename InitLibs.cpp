#include <stdexcept>
#include "InitLibs.h"
// #include "glfw3.h"
#include "SDL.h"
#include "fterrors.h"

namespace VKKit {
// GLFW::GLFW()
// {
//     if (glfwInit() == GLFW_FALSE)
//         throw std::runtime_error("Failed to initialise GLFW");
// }

// GLFW::~GLFW()
// {
//     glfwTerminate();
// }

SDL::SDL()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        char error[256];
        snprintf(error, sizeof(error), "Failed to initialise SDL. Error: %s", SDL_GetError());
        throw std::runtime_error(error);
    }
}

SDL::~SDL()
{
    SDL_Quit();
}

FreeType::FreeType()
{
    const auto result = FT_Init_FreeType(&ft);

    if (result) {
        char error[256];
        snprintf(error, sizeof(error), "Failed to initalise the FreeType library. Error: %s", FT_Error_String(result));
        throw std::runtime_error(error);
    }
}

FreeType::~FreeType()
{
    FT_Done_FreeType(ft);
}
}