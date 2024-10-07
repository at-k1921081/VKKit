#ifndef RENDERDATA_H
#define RENDERDATA_H

#include <vector>
#include <istream>
#include <ostream>
#include "glm/glm.hpp"

namespace VKKit {
class DescriptorPool;

struct Pos2D {
    float x, y;
};

struct Pos3D {
    float x, y, z;
};

struct Rect {
    float x, y, w, h;
};

struct Cuboid {
    float x, y, z, w, h, d;
};

struct Color {
    float r, g, b, a;
};

inline std::istream& operator>>(std::istream& is, Pos2D& p) { return is >> p.x >> p.y; }
inline std::istream& operator>>(std::istream& is, Pos3D& p) { return is >> p.x >> p.y >> p.z; }
inline std::istream& operator>>(std::istream& is, Rect& r) { return is >> r.x >> r.y >> r.w >> r.h; }
inline std::istream& operator>>(std::istream& is, Cuboid& c) { return is >> c.x >> c.y >> c.z >> c.w >> c.h >> c.d; }
inline std::istream& operator>>(std::istream& is, Color& c) { return is >> c.r >> c.g >> c.b >> c.a; }

inline std::ostream& operator<<(std::ostream& os, const Pos2D& p) { return os << p.x << ' ' << p.y << ' '; }
inline std::ostream& operator<<(std::ostream& os, const Pos3D& p) { return os << p.x << ' ' << p.y << ' ' << p.z << ' '; }
inline std::ostream& operator<<(std::ostream& os, const Rect& r) { return os << r.x << ' ' << r.y << ' ' << r.w << ' ' << r.h << ' '; }
inline std::ostream& operator<<(std::ostream& os, const Cuboid& c) { return os << c.x << ' ' << c.y << ' ' << c.z << ' ' << c.w << ' ' <<
    c.h << ' ' << c.d << ' '; }
inline std::ostream& operator<<(std::ostream& os, const Color& c) { return os << c.r << ' ' << c.g << ' ' << c.b << ' ' << c.a << ' '; }

constexpr bool PosInRect(Pos2D pos, Rect rect) noexcept
{
    return (pos.x >= rect.x && pos.x <= rect.x + rect.w &&
            pos.y >= rect.y && pos.y <= rect.y + rect.h);
}

constexpr bool HasIntersection(const Rect& r1, const Rect& r2) noexcept
{
    return (r1.x > r2.x - r1.w && r1.x < r2.x + r2.w &&
            r1.y > r2.y - r1.h && r1.y < r2.y + r2.h);
}

constexpr bool PosInCuboid(Pos3D pos, const Cuboid& cuboid) noexcept
{
    return (pos.x >= cuboid.x && pos.x <= cuboid.x + cuboid.w &&
            pos.y >= cuboid.y && pos.y <= cuboid.y + cuboid.h &&
            pos.z >= cuboid.z && pos.z <= cuboid.z + cuboid.d);
}

constexpr bool HasIntersection(const Cuboid& c1, const Cuboid& c2) noexcept
{
    return (c1.x > c2.x - c1.w && c1.x < c2.x + c2.w &&
            c1.y > c2.y - c1.h && c1.y < c2.y + c2.h &&
            c1.z > c2.z - c1.d && c1.z < c2.z + c2.d);
}

// Calculate the distance between two 2D points
inline float Distance(Pos2D p1, Pos2D p2) noexcept
{
    return sqrt((p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y));
}

// Calculate the distance between two rectangles
// constexpr float Distance(const Rect& r1, const Rect& r2)
// {
//     const float r1_left = r1.x;
//     const float r1_right = r1.x + r1.w;

//     const bool above_or_below = ;

//     if (r1.x > r2.x && r1.x < r2.x + r2.w) {

//     }
// }

inline Pos2D NormalizedPos(Pos2D pos) noexcept
{
    const float magnitute = sqrt(pos.x * pos.x + pos.y * pos.y);
    pos.x /= magnitute;
    pos.y /= magnitute;
    return pos;
}

constexpr Pos2D CenterOfRect(const VKKit::Rect& r)
{
    return { r.x + r.w / 2.0f, r.y + r.h / 2.0f };
}

struct CameraView {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 tex_coord;

    constexpr bool operator==(const Vertex& other) const noexcept
    {
        return pos == other.pos && color == other.color && tex_coord == other.tex_coord;
    }
};

struct RenderData {
    std::vector<float> vertices;
    std::vector<uint32_t> indices;

    static RenderData CreateTexturedRect(Rect r);
    static RenderData CreateColoredRect(Color color, Rect area);
};
}

#endif