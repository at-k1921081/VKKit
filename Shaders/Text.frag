#version 450

layout (location = 0) in vec2 texPos;

layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform sampler2D bitmap;
layout (binding = 1) uniform TextColor { vec4 value; } color;

void main()
{
    outColor = color.value * vec4(1.0, 1.0, 1.0, texture(bitmap, texPos).r);
    // outColor = color.value * texture(bitmap, texPos);
}