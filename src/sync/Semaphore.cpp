#include "fusion/sync/Semaphore.hpp"
#include "fusion/core/Device.hpp"
#include <stdexcept>

namespace fusion {

Semaphore::Semaphore(const Device& device)
    : m_device(device.handle())
{
    VkSemaphoreCreateInfo ci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    if (vkCreateSemaphore(m_device, &ci, nullptr, &m_semaphore) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create semaphore");
}

Semaphore::~Semaphore()
{
    if (m_semaphore) vkDestroySemaphore(m_device, m_semaphore, nullptr);
}

Semaphore::Semaphore(Semaphore&& o) noexcept
    : m_device(o.m_device), m_semaphore(o.m_semaphore)
{
    o.m_semaphore = VK_NULL_HANDLE;
}

Semaphore& Semaphore::operator=(Semaphore&& o) noexcept
{
    if (this != &o) {
        if (m_semaphore) vkDestroySemaphore(m_device, m_semaphore, nullptr);
        m_device    = o.m_device;
        m_semaphore = o.m_semaphore;
        o.m_semaphore = VK_NULL_HANDLE;
    }
    return *this;
}

} // namespace fusion
