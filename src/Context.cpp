#include "fusion/Context.hpp"

namespace fusion {

Context::Context(GLFWwindow* window, const ContextConfig& cfg)
{
    m_instance  = std::make_unique<Instance>(cfg.instance);
    m_surface   = std::make_unique<Surface>(m_instance->handle(), window);
    m_physDevice = PhysicalDevice::pick(m_instance->handle(),
                                        m_surface->handle(),
                                        cfg.deviceExtensions);

    VkPhysicalDeviceFeatures features{};
    features.samplerAnisotropy = VK_TRUE;
    features.fillModeNonSolid  = VK_TRUE;
    features.geometryShader    = m_physDevice.features().geometryShader;

    m_device    = std::make_unique<Device>(m_physDevice, cfg.deviceExtensions, features);
    m_swapchain = std::make_unique<Swapchain>(*m_device, m_physDevice,
                                              m_surface->handle(), window, cfg.swapchain);
    m_allocator = std::make_unique<Allocator>(*m_instance, m_physDevice, *m_device);
}

void Context::onResize(GLFWwindow* window)
{
    m_device->waitIdle();
    m_swapchain->recreate(window);
}

} // namespace fusion
