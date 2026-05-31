#include "fusion/core/PhysicalDevice.hpp"
#include <stdexcept>
#include <algorithm>
#include <cstring>

namespace fusion {

PhysicalDevice::PhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface)
    : m_device(device)
{
    vkGetPhysicalDeviceProperties(m_device, &m_props);
    vkGetPhysicalDeviceFeatures(m_device, &m_features);

    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(m_device, nullptr, &count, nullptr);
    m_extensions.resize(count);
    vkEnumerateDeviceExtensionProperties(m_device, nullptr, &count, m_extensions.data());

    // Queue families
    vkGetPhysicalDeviceQueueFamilyProperties(m_device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_device, &count, families.data());

    for (uint32_t i = 0; i < count; ++i) {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            m_indices.graphics = i;
        if (families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            m_indices.compute = i;
        if ((families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            !(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            m_indices.transfer = i;

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_device, i, surface, &presentSupport);
        if (presentSupport)
            m_indices.present = i;
    }

    // Fall back transfer to graphics family if dedicated not found
    if (!m_indices.transfer.has_value())
        m_indices.transfer = m_indices.graphics;
}

SwapchainSupport PhysicalDevice::querySwapchainSupport(VkSurfaceKHR surface) const
{
    SwapchainSupport s;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device, surface, &s.capabilities);

    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_device, surface, &count, nullptr);
    s.formats.resize(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_device, surface, &count, s.formats.data());

    vkGetPhysicalDeviceSurfacePresentModesKHR(m_device, surface, &count, nullptr);
    s.presentModes.resize(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_device, surface, &count, s.presentModes.data());

    return s;
}

bool PhysicalDevice::supportsExtension(const char* name) const
{
    return std::any_of(m_extensions.begin(), m_extensions.end(),
        [name](const VkExtensionProperties& e) {
            return std::strcmp(e.extensionName, name) == 0;
        });
}

uint32_t PhysicalDevice::findMemoryType(uint32_t filter, VkMemoryPropertyFlags flags) const
{
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_device, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
        if ((filter & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    throw std::runtime_error("fusion: no suitable memory type");
}

VkFormat PhysicalDevice::findSupportedFormat(const std::vector<VkFormat>& candidates,
    VkImageTiling tiling, VkFormatFeatureFlags features) const
{
    for (VkFormat fmt : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_device, fmt, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR  && (props.linearTilingFeatures  & features) == features)
            return fmt;
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return fmt;
    }
    throw std::runtime_error("fusion: no supported format found");
}

VkFormat PhysicalDevice::findDepthFormat() const
{
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

int PhysicalDevice::score() const
{
    int s = 0;
    if (m_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) s += 1000;
    s += static_cast<int>(m_props.limits.maxImageDimension2D / 100);
    if (!m_indices.isComplete()) s = -1;
    return s;
}

PhysicalDevice PhysicalDevice::pick(VkInstance instance, VkSurfaceKHR surface,
    const std::vector<const char*>& requiredExtensions)
{
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0)
        throw std::runtime_error("fusion: no Vulkan-capable GPU found");

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());

    PhysicalDevice best;
    int bestScore = -1;

    for (auto d : devices) {
        PhysicalDevice pd(d, surface);
        bool hasExts = std::all_of(requiredExtensions.begin(), requiredExtensions.end(),
            [&pd](const char* ext) { return pd.supportsExtension(ext); });
        if (!hasExts) continue;

        auto sc = pd.querySwapchainSupport(surface);
        if (sc.formats.empty() || sc.presentModes.empty()) continue;

        int s = pd.score();
        if (s > bestScore) { bestScore = s; best = std::move(pd); }
    }

    if (bestScore < 0)
        throw std::runtime_error("fusion: no suitable physical device found");
    return best;
}

} // namespace fusion
