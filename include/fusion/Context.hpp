#pragma once
#include "core/Instance.hpp"
#include "core/PhysicalDevice.hpp"
#include "core/Device.hpp"
#include "core/Surface.hpp"
#include "core/Swapchain.hpp"
#include "resources/Allocator.hpp"
#include <GLFW/glfw3.h>
#include <memory>

namespace fusion {

struct ContextConfig {
    InstanceConfig   instance;
    SwapchainConfig  swapchain;
    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
};

// Owns the entire Vulkan context; everything else borrows from it.
class Context {
public:
    explicit Context(GLFWwindow* window, const ContextConfig& cfg = {});
    ~Context() = default;

    Context(const Context&)            = delete;
    Context& operator=(const Context&) = delete;

    const Instance&        instance()    const { return *m_instance; }
    const PhysicalDevice&  physDevice()  const { return m_physDevice; }
    const Device&          device()      const { return *m_device; }
    const Surface&         surface()     const { return *m_surface; }
    Swapchain&             swapchain()         { return *m_swapchain; }
    const Swapchain&       swapchain()   const { return *m_swapchain; }
    const Allocator&       allocator()   const { return *m_allocator; }

    void onResize(GLFWwindow* window);

private:
    std::unique_ptr<Instance>    m_instance;
    PhysicalDevice               m_physDevice;
    std::unique_ptr<Surface>     m_surface;
    std::unique_ptr<Device>      m_device;
    std::unique_ptr<Swapchain>   m_swapchain;
    std::unique_ptr<Allocator>   m_allocator;
};

} // namespace fusion
