#include "fusion/core/Device.hpp"
#include <stdexcept>
#include <set>
#include <vector>

namespace fusion {

Device::Device(const PhysicalDevice& physDevice,
               const std::vector<const char*>& extensions,
               const VkPhysicalDeviceFeatures& enabledFeatures)
{
    const auto& idx = physDevice.queueFamilies();

    std::set<uint32_t> uniqueFamilies = {
        idx.graphics.value(),
        idx.present.value(),
        idx.compute.value_or(idx.graphics.value()),
        idx.transfer.value_or(idx.graphics.value()),
    };

    float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCIs;
    for (uint32_t family : uniqueFamilies) {
        VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        qci.queueFamilyIndex = family;
        qci.queueCount       = 1;
        qci.pQueuePriorities = &priority;
        queueCIs.push_back(qci);
    }

    VkDeviceCreateInfo ci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    ci.queueCreateInfoCount    = static_cast<uint32_t>(queueCIs.size());
    ci.pQueueCreateInfos       = queueCIs.data();
    ci.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    ci.ppEnabledExtensionNames = extensions.data();
    ci.pEnabledFeatures        = &enabledFeatures;

    if (vkCreateDevice(physDevice.handle(), &ci, nullptr, &m_device) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create logical device");

    auto getQueue = [&](std::optional<uint32_t> family) -> Queue {
        uint32_t f = family.value_or(idx.graphics.value());
        VkQueue q = VK_NULL_HANDLE;
        vkGetDeviceQueue(m_device, f, 0, &q);
        return Queue(q, f);
    };

    m_graphics = getQueue(idx.graphics);
    m_present  = getQueue(idx.present);
    m_compute  = getQueue(idx.compute);
    m_transfer = getQueue(idx.transfer);
}

Device::~Device()
{
    vkDestroyDevice(m_device, nullptr);
}

void Device::waitIdle() const
{
    vkDeviceWaitIdle(m_device);
}

VkCommandBuffer Device::beginSingleTimeCommands(VkCommandPool pool) const
{
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool        = pool;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(m_device, &ai, &cmd);

    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &bi);
    return cmd;
}

void Device::endSingleTimeCommands(VkCommandBuffer cmd, VkCommandPool pool, VkQueue queue) const
{
    vkEndCommandBuffer(cmd);

    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1;
    si.pCommandBuffers    = &cmd;
    vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(m_device, pool, 1, &cmd);
}

void Device::setDebugName(VkObjectType type, uint64_t obj, const char* name) const
{
#ifdef FUSION_ENABLE_DEBUG_MARKERS
    auto fn = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
        vkGetDeviceProcAddr(m_device, "vkSetDebugUtilsObjectNameEXT"));
    if (!fn) return;
    VkDebugUtilsObjectNameInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
    info.objectType   = type;
    info.objectHandle = obj;
    info.pObjectName  = name;
    fn(m_device, &info);
#else
    (void)type; (void)obj; (void)name;
#endif
}

} // namespace fusion
