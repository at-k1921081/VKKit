#version 450

layout (binding = 0) uniform CameraView {
    mat4 model;
    mat4 view;
    mat4 proj;
} cv;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;

layout (location = 0) out vec4 fragColor;

void main()
{
    gl_Position = cv.proj * cv.view * cv.model * vec4(aPos, 1.0);

    fragColor = aColor;
}