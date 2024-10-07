#ifndef INITLIBS_H
#define INITLIBS_H

#include "freetype.h"

namespace VKKit {
// class GLFW {
// public:
//     GLFW();
//     ~GLFW();
// };

class SDL {
public:
    SDL();
    ~SDL();
};

class FreeType {
public:
    FreeType();
    ~FreeType();

    FreeType(const FreeType&) = delete;
    FreeType& operator=(const FreeType&) = delete;
    FreeType(FreeType&&) = delete;
    FreeType& operator=(FreeType&&) = delete;

    FT_Library Get() const noexcept { return ft; }

private:
    FT_Library ft;
};
}

#endif