cmake_minimum_required(VERSION 3.20)
project(fusion VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

option(FUSION_BUILD_EXAMPLES  "Build example applications"  ON)
option(FUSION_BUILD_TESTS     "Build unit/integration tests" ON)
option(FUSION_BUILD_BENCH     "Build benchmarks"             OFF)
option(FUSION_BUILD_TOOLS     "Build developer tools"        ON)

# VMA — header-only, lives in third_party/
add_library(vma INTERFACE)
target_include_directories(vma INTERFACE third_party/VulkanMemoryAllocator/include)

add_library(fusion
    src/core/Instance.cpp
    src/core/Device.cpp
    src/core/PhysicalDevice.cpp
    src/core/Surface.cpp
    src/core/Swapchain.cpp
    src/core/Queue.cpp
    src/resources/Buffer.cpp
    src/resources/Image.cpp
    src/resources/Sampler.cpp
    src/resources/Shader.cpp
    src/resources/Allocator.cpp
    src/render/RenderPass.cpp
    src/render/Pipeline.cpp
    src/render/PipelineCache.cpp
    src/render/Framebuffer.cpp
    src/render/CommandBuffer.cpp
    src/render/CommandPool.cpp
    src/render/DescriptorSet.cpp
    src/render/DescriptorPool.cpp
    src/render/FrameGraph.cpp
    src/sync/Fence.cpp
    src/sync/Semaphore.cpp
    src/sync/Timeline.cpp
    src/Context.cpp
    src/Renderer.cpp
)

target_include_directories(fusion
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        # VMA is included from public headers (Allocator.hpp, Buffer.hpp), so
        # consumers need the path too.
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/VulkanMemoryAllocator/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
)

target_link_libraries(fusion
    PUBLIC
        Vulkan::Vulkan
        glfw
        glm::glm
)

target_compile_definitions(fusion
    PUBLIC
        GLM_FORCE_RADIANS
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_ENABLE_EXPERIMENTAL
    PRIVATE
        GLFW_INCLUDE_VULKAN
)

target_compile_options(fusion PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(fusion PUBLIC FUSION_ENABLE_VALIDATION FUSION_ENABLE_DEBUG_MARKERS)
endif()

if(FUSION_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(FUSION_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(FUSION_BUILD_BENCH)
    add_subdirectory(bench)
endif()

if(FUSION_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

# Shader compilation
add_subdirectory(shaders)
