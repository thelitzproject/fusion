#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace fusion {

class Surface {
public:
    Surface(VkInstance instance, GLFWwindow* window);
    ~Surface();

    Surface(const Surface&)            = delete;
    Surface& operator=(const Surface&) = delete;

    VkSurfaceKHR handle() const { return m_surface; }

private:
    VkInstance   m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface  = VK_NULL_HANDLE;
};

} // namespace fusion
