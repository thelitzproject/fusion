# Integration tests use Vulkan + VMA directly (no GLFW/fusion link) so they
# run safely in headless CI environments without crashing on display init.
add_executable(integration_tests test_context.cpp)

target_include_directories(integration_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/third_party/VulkanMemoryAllocator/include"
)

target_compile_definitions(integration_tests PRIVATE VMA_IMPLEMENTATION)

target_link_libraries(integration_tests PRIVATE Vulkan::Vulkan)

add_test(NAME integration_tests COMMAND integration_tests)
