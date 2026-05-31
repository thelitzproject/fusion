#include "fusion/core/Swapchain.hpp"
#include "fusion/core/Device.hpp"
#include <algorithm>
#include <stdexcept>
#include <limits>

namespace fusion {

Swapchain::Swapchain(const Device& device, const PhysicalDevice& physDevice,
                     VkSurfaceKHR surface, GLFWwindow* window, const SwapchainConfig& cfg)
    : m_device(device), m_physDevice(physDevice), m_surface(surface), m_cfg(cfg)
{
    create(window);
}

Swapchain::~Swapchain()
{
    destroy();
}

void Swapchain::create(GLFWwindow* window)
{
    auto support = m_physDevice.querySwapchainSupport(m_surface);
    auto fmt     = chooseFormat(support.formats);
    auto mode    = chooseMode(support.presentModes);
    auto extent  = chooseExtent(support.capabilities, window);

    m_format = fmt.format;
    m_extent = extent;

    uint32_t imageCount = std::clamp(m_cfg.imageCount,
        support.capabilities.minImageCount,
        support.capabilities.maxImageCount > 0
            ? support.capabilities.maxImageCount
            : std::numeric_limits<uint32_t>::max());

    VkSwapchainCreateInfoKHR ci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    ci.surface          = m_surface;
    ci.minImageCount    = imageCount;
    ci.imageFormat      = fmt.format;
    ci.imageColorSpace  = fmt.colorSpace;
    ci.imageExtent      = extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const auto& idx = m_physDevice.queueFamilies();
    uint32_t families[] = { idx.graphics.value(), idx.present.value() };
    if (families[0] != families[1]) {
        ci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount = 2;
        ci.pQueueFamilyIndices   = families;
    } else {
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    ci.preTransform   = support.capabilities.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode    = mode;
    ci.clipped        = VK_TRUE;

    if (vkCreateSwapchainKHR(m_device.handle(), &ci, nullptr, &m_swapchain) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create swapchain");

    uint32_t count = 0;
    vkGetSwapchainImagesKHR(m_device.handle(), m_swapchain, &count, nullptr);
    m_images.resize(count);
    vkGetSwapchainImagesKHR(m_device.handle(), m_swapchain, &count, m_images.data());

    m_imageViews.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vci.image    = m_images[i];
        vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vci.format   = m_format;
        vci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        if (vkCreateImageView(m_device.handle(), &vci, nullptr, &m_imageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("fusion: failed to create swapchain image view");
    }
}

void Swapchain::destroy()
{
    for (auto v : m_imageViews)
        vkDestroyImageView(m_device.handle(), v, nullptr);
    m_imageViews.clear();
    m_images.clear();
    vkDestroySwapchainKHR(m_device.handle(), m_swapchain, nullptr);
    m_swapchain = VK_NULL_HANDLE;
}

void Swapchain::recreate(GLFWwindow* window)
{
    m_device.waitIdle();
    destroy();
    create(window);
}

VkResult Swapchain::acquireNextImage(VkSemaphore signal, uint32_t& outIndex) const
{
    return vkAcquireNextImageKHR(m_device.handle(), m_swapchain,
        UINT64_MAX, signal, VK_NULL_HANDLE, &outIndex);
}

VkResult Swapchain::present(VkSemaphore wait, uint32_t imageIndex) const
{
    VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores    = &wait;
    pi.swapchainCount     = 1;
    pi.pSwapchains        = &m_swapchain;
    pi.pImageIndices      = &imageIndex;
    return vkQueuePresentKHR(m_device.present().handle(), &pi);
}

VkSurfaceFormatKHR Swapchain::chooseFormat(const std::vector<VkSurfaceFormatKHR>& formats) const
{
    for (auto& f : formats)
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;
    return formats[0];
}

VkPresentModeKHR Swapchain::chooseMode(const std::vector<VkPresentModeKHR>& modes) const
{
    if (!m_cfg.vsync)
        for (auto m : modes)
            if (m == VK_PRESENT_MODE_IMMEDIATE_KHR) return m;
    for (auto m : modes)
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseExtent(const VkSurfaceCapabilitiesKHR& caps, GLFWwindow* window) const
{
    if (caps.currentExtent.width != UINT32_MAX)
        return caps.currentExtent;

    int w = 0, h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    return {
        std::clamp(static_cast<uint32_t>(w), caps.minImageExtent.width,  caps.maxImageExtent.width),
        std::clamp(static_cast<uint32_t>(h), caps.minImageExtent.height, caps.maxImageExtent.height)
    };
}

} // namespace fusion
