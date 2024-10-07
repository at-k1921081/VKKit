cmake_minimum_required(VERSION 3.25.0)

target_sources(VKKit PRIVATE    Color2DPipeline.cpp
                                Color3DPipeline.cpp
                                Texture2DPipeline.cpp
                                Texture3DPipeline.cpp
                                TextPipeline.cpp)

target_compile_options(VKKit PUBLIC -Wall -Wextra -Werror -Wno-unused-parameter)
target_compile_features(VKKit PUBLIC cxx_std_23)