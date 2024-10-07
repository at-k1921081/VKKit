#include <array>
#include "RenderData.h"
#include "vulkan/vulkan.h"

namespace VKKit {
RenderData RenderData::CreateTexturedRect(Rect r)
{
    return RenderData {
        .vertices = {
            r.x,       r.y,       0.0f, 0.0f, 0.0f,
            r.x + r.w, r.y,       0.0f, 1.0f, 0.0f,
            r.x + r.w, r.y + r.h, 0.0f, 1.0f, 1.0f,
            r.x,       r.y + r.h, 0.0f, 0.0f, 1.0f
        },
        .indices = {
            0, 1, 2, 2, 3, 0
        }
    };
}

RenderData RenderData::CreateColoredRect(Color color, Rect area)
{
    return RenderData {
        .vertices = {
            area.x,          area.y,          0.0f, color.r, color.g, color.b, color.a,
            area.x + area.w, area.y,          0.0f, color.r, color.g, color.b, color.a,
            area.x + area.w, area.y + area.h, 0.0f, color.r, color.g, color.b, color.a,
            area.x,          area.y + area.h, 0.0f, color.r, color.g, color.b, color.a,
        },
        .indices = {
            0, 1, 2, 2, 3, 0
        }
    };
}
}