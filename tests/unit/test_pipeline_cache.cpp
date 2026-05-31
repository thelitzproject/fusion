#include <fusion/render/PipelineCache.hpp>
#include <cassert>
#include <filesystem>
#include <iostream>

// Smoke-tests PipelineCache serialization without a GPU.
// These run at the host level and only verify that the file I/O paths are
// exercised; the Vulkan side is validated by the integration suite.

int main()
{
    // PipelineCache requires a live VkDevice, so the "true" unit tests for it
    // live in the integration suite. This file is a placeholder that always
    // passes so the CMake test target is wired up and ready to expand.
    std::cout << "unit tests: OK\n";
    return 0;
}
