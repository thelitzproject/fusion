#include "fusion/sync/Fence.hpp"
#include "fusion/core/Device.hpp"
#include <stdexcept>

namespace fusion {

Fence::Fence(const Device& device, bool signaled)
    : m_device(device.handle())
{
    VkFenceCreateInfo ci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    if (signaled) ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateFence(m_device, &ci, nullptr, &m_fence) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create fence");
}

Fence::~Fence()
{
    if (m_fence) vkDestroyFence(m_device, m_fence, nullptr);
}

Fence::Fence(Fence&& o) noexcept
    : m_device(o.m_device), m_fence(o.m_fence)
{
    o.m_fence = VK_NULL_HANDLE;
}

Fence& Fence::operator=(Fence&& o) noexcept
{
    if (this != &o) {
        if (m_fence) vkDestroyFence(m_device, m_fence, nullptr);
        m_device = o.m_device;
        m_fence  = o.m_fence;
        o.m_fence = VK_NULL_HANDLE;
    }
    return *this;
}

void Fence::wait(uint64_t timeout) const
{
    vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, timeout);
}

void Fence::reset()
{
    vkResetFences(m_device, 1, &m_fence);
}

bool Fence::isSignaled() const
{
    return vkGetFenceStatus(m_device, m_fence) == VK_SUCCESS;
}

} // namespace fusion
