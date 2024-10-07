#version 450

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexPos;

layout (location = 0) out vec2 texPos;

layout (binding = 2) uniform Projection {
    mat4 projection;
} pm;

void main()
{
    gl_Position = pm.projection * vec4(aPos, 0.0, 1.0);
    texPos = aTexPos;
}