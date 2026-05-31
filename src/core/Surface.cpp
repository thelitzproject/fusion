#include "fusion/core/Surface.hpp"
#include <stdexcept>

namespace fusion {

Surface::Surface(VkInstance instance, GLFWwindow* window)
    : m_instance(instance)
{
    if (glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create window surface");
}

Surface::~Surface()
{
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

} // namespace fusion
