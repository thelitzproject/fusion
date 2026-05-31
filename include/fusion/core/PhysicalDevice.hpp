#pragma once
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>

namespace fusion {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
    std::optional<uint32_t> compute;
    std::optional<uint32_t> transfer;

    bool isComplete() const {
        return graphics.has_value() && present.has_value();
    }
};

struct SwapchainSupport {
    VkSurfaceCapabilitiesKHR        capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

class PhysicalDevice {
public:
    PhysicalDevice() = default;
    PhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface);

    VkPhysicalDevice              handle()       const { return m_device; }
    const QueueFamilyIndices&     queueFamilies() const { return m_indices; }
    const VkPhysicalDeviceProperties& properties() const { return m_props; }
    const VkPhysicalDeviceFeatures&   features()   const { return m_features; }

    SwapchainSupport querySwapchainSupport(VkSurfaceKHR surface) const;
    bool             supportsExtension(const char* name) const;
    uint32_t         findMemoryType(uint32_t filter, VkMemoryPropertyFlags flags) const;
    VkFormat         findSupportedFormat(const std::vector<VkFormat>& candidates,
                                         VkImageTiling tiling,
                                         VkFormatFeatureFlags features) const;
    VkFormat         findDepthFormat() const;

    static PhysicalDevice pick(VkInstance instance, VkSurfaceKHR surface,
                                const std::vector<const char*>& requiredExtensions);

private:
    int score() const;

    VkPhysicalDevice               m_device  = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties     m_props{};
    VkPhysicalDeviceFeatures       m_features{};
    QueueFamilyIndices             m_indices{};
    std::vector<VkExtensionProperties> m_extensions;
};

} // namespace fusion
