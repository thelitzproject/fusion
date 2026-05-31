#pragma once
#include "PhysicalDevice.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <functional>

namespace fusion {

class Queue {
public:
    Queue() = default;
    Queue(VkQueue queue, uint32_t familyIndex)
        : m_queue(queue), m_familyIndex(familyIndex) {}

    VkQueue  handle()      const { return m_queue; }
    uint32_t familyIndex() const { return m_familyIndex; }

    void submit(const VkSubmitInfo& info, VkFence fence = VK_NULL_HANDLE) const;
    void waitIdle() const;

private:
    VkQueue  m_queue       = VK_NULL_HANDLE;
    uint32_t m_familyIndex = 0;
};

class Device {
public:
    Device(const PhysicalDevice& physDevice,
           const std::vector<const char*>& extensions,
           const VkPhysicalDeviceFeatures& enabledFeatures);
    ~Device();

    Device(const Device&)            = delete;
    Device& operator=(const Device&) = delete;

    VkDevice         handle()    const { return m_device; }
    const Queue&     graphics()  const { return m_graphics; }
    const Queue&     present()   const { return m_present; }
    const Queue&     compute()   const { return m_compute; }
    const Queue&     transfer()  const { return m_transfer; }

    void waitIdle() const;

    // One-shot command helpers
    VkCommandBuffer beginSingleTimeCommands(VkCommandPool pool) const;
    void            endSingleTimeCommands(VkCommandBuffer cmd,
                                          VkCommandPool pool,
                                          VkQueue queue) const;

    void setDebugName(VkObjectType type, uint64_t handle, const char* name) const;

private:
    VkDevice m_device   = VK_NULL_HANDLE;
    Queue    m_graphics;
    Queue    m_present;
    Queue    m_compute;
    Queue    m_transfer;
};

} // namespace fusion
