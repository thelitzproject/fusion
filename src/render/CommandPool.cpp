#include "fusion/render/CommandBuffer.hpp"
#include "fusion/core/Device.hpp"
#include <stdexcept>

namespace fusion {

CommandPool::CommandPool(const Device& device, uint32_t queueFamily,
    VkCommandPoolCreateFlags flags)
    : m_device(device.handle())
{
    VkCommandPoolCreateInfo ci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    ci.queueFamilyIndex = queueFamily;
    ci.flags            = flags;
    if (vkCreateCommandPool(m_device, &ci, nullptr, &m_pool) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create command pool");
}

CommandPool::~CommandPool()
{
    if (m_pool) vkDestroyCommandPool(m_device, m_pool, nullptr);
}

CommandPool::CommandPool(CommandPool&& o) noexcept
    : m_device(o.m_device), m_pool(o.m_pool)
{
    o.m_pool = VK_NULL_HANDLE;
}

CommandPool& CommandPool::operator=(CommandPool&& o) noexcept
{
    if (this != &o) {
        if (m_pool) vkDestroyCommandPool(m_device, m_pool, nullptr);
        m_device = o.m_device;
        m_pool   = o.m_pool;
        o.m_pool = VK_NULL_HANDLE;
    }
    return *this;
}

void CommandPool::reset(VkCommandPoolResetFlags flags)
{
    vkResetCommandPool(m_device, m_pool, flags);
}

} // namespace fusion
