#pragma once
#include "PhysicalDevice.hpp"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <functional>

namespace fusion {

class Device;

struct SwapchainConfig {
    uint32_t preferredWidth  = 1280;
    uint32_t preferredHeight = 720;
    bool     vsync           = true;
    uint32_t imageCount      = 2;
};

class Swapchain {
public:
    Swapchain(const Device& device,
              const PhysicalDevice& physDevice,
              VkSurfaceKHR surface,
              GLFWwindow* window,
              const SwapchainConfig& cfg = {});
    ~Swapchain();

    Swapchain(const Swapchain&)            = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    VkSwapchainKHR           handle()      const { return m_swapchain; }
    VkFormat                 format()      const { return m_format; }
    VkExtent2D               extent()      const { return m_extent; }
    uint32_t                 imageCount()  const { return static_cast<uint32_t>(m_images.size()); }
    VkImage                  image(uint32_t i)     const { return m_images[i]; }
    VkImageView              imageView(uint32_t i) const { return m_imageViews[i]; }

    // Returns VK_ERROR_OUT_OF_DATE_KHR when resize needed
    VkResult acquireNextImage(VkSemaphore signal, uint32_t& outIndex) const;
    VkResult present(VkSemaphore wait, uint32_t imageIndex) const;

    void recreate(GLFWwindow* window);

private:
    void create(GLFWwindow* window);
    void destroy();

    VkSurfaceFormatKHR chooseFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
    VkPresentModeKHR   chooseMode(const std::vector<VkPresentModeKHR>& modes) const;
    VkExtent2D         chooseExtent(const VkSurfaceCapabilitiesKHR& caps, GLFWwindow* window) const;

    const Device&         m_device;
    const PhysicalDevice& m_physDevice;
    VkSurfaceKHR          m_surface    = VK_NULL_HANDLE;
    VkSwapchainKHR        m_swapchain  = VK_NULL_HANDLE;
    VkFormat              m_format{};
    VkExtent2D            m_extent{};
    SwapchainConfig       m_cfg{};

    std::vector<VkImage>     m_images;
    std::vector<VkImageView> m_imageViews;
};

} // namespace fusion
