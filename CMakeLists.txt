cmake_minimum_required(VERSION 3.25.0)

project(VKKit)

add_library(VKKit   InitLibs.cpp
                    InitLibs.h
                    Windowing.cpp
                    Windowing.h
                    Context.cpp
                    Context.h
                    Constants.h
                    VkResultString.h
                    Concurrency.cpp
                    Concurrency.h
                    Buffer.cpp
                    Buffer.h
                    BufferVec.cpp
                    BufferVec.h
                    Debugger.cpp
                    Debugger.h
                    CommandBuffer.cpp
                    CommandBuffer.h
                    Texture.cpp
                    Texture.h
                    Model.cpp
                    Model.h
                    Alphabet.cpp
                    Alphabet.h
                    GraphicsPipeline.cpp
                    GraphicsPipeline.h
                    VulkanObjects.h
                    Instance.cpp
                    Instance.h
                    Surface.cpp
                    Surface.h
                    PhysicalDevice.cpp
                    PhysicalDevice.h
                    Device.cpp
                    Device.h
                    RenderPass.cpp
                    RenderPass.h
                    DescriptorSetLayout.cpp
                    DescriptorSetLayout.h
                    DescriptorPool.cpp
                    DescriptorPool.h
                    CommandPool.cpp
                    CommandPool.h
                    Sampler.cpp
                    Sampler.h
                    Swapchain.cpp
                    Swapchain.h
                    ImageObjects.cpp
                    ImageObjects.h
                    DefaultConfigurations.h
                    RenderData.cpp
                    RenderData.h)

if (MSVC)

else()
target_compile_options(VKKit PUBLIC -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-field-initializers)
target_compile_features(VKKit PUBLIC cxx_std_23)
endif()

add_subdirectory(DefaultConfigurations)

if (WIN32)
    set(INCLUDE "C:/Users/Alex/Include")

    target_include_directories(VKKit SYSTEM PUBLIC
    "${INCLUDE}/glm"
    "${INCLUDE}/Vulkan/Include"
    "C:/Users/Alex/Desktop/Development/ProjectsLibraries/Utils"
    "${INCLUDE}/STB"
    "${INCLUDE}/TinyObjLoader"
    "${INCLUDE}/Assimp/assimp/include/assimp"
    "${INCLUDE}/Assimp/assimp/include/"
    "${INCLUDE}/Assimp/assimp/build/include"
    "${INCLUDE}/freetype/include/"
    "${INCLUDE}/freetype/include/freetype"
    "${INCLUDE}/SDL2/include")

    target_link_directories(VKKit PUBLIC
    "${INCLUDE}/Vulkan/Lib"
    "C:/Users/Alex/Desktop/Development/ProjectsLibraries/Utils/build"
    "${INCLUDE}/freetype/build"
    "${INCLUDE}/Assimp/assimp/build/lib"
    "${INCLUDE}/SDL2/lib/x64")

    target_link_libraries(VKKit PUBLIC freetype vulkan-1 assimp Utils SDL2main SDL2)
elseif (UNIX)
    target_include_directories(VKKit PUBLIC
    "/usr/include/SDL2"
    "/usr/include/assimp"
    "/usr/include/freetype2"
    "/usr/include/freetype2/freetype"
    "~/Desktop/Development/Utils")
    target_link_directories(VKKit PUBLIC
    "/usr/lib/"
    "~/Desktop/Development/Utils/build")
    target_link_libraries(VKKit freetype vulkan Utils GameWidgets)
endif ()