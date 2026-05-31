#include "fusion/sync/Semaphore.hpp"
#include "fusion/core/Device.hpp"
#include <stdexcept>

namespace fusion {

TimelineSemaphore::TimelineSemaphore(const Device& device, uint64_t initialValue)
    : m_device(device.handle())
{
    VkSemaphoreTypeCreateInfo typeCI{VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
    typeCI.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    typeCI.initialValue  = initialValue;

    VkSemaphoreCreateInfo ci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    ci.pNext = &typeCI;

    if (vkCreateSemaphore(m_device, &ci, nullptr, &m_semaphore) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create timeline semaphore");
}

TimelineSemaphore::~TimelineSemaphore()
{
    vkDestroySemaphore(m_device, m_semaphore, nullptr);
}

uint64_t TimelineSemaphore::value() const
{
    uint64_t val = 0;
    vkGetSemaphoreCounterValue(m_device, m_semaphore, &val);
    return val;
}

void TimelineSemaphore::wait(uint64_t value, uint64_t timeout) const
{
    VkSemaphoreWaitInfo wi{VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
    wi.semaphoreCount = 1;
    wi.pSemaphores    = &m_semaphore;
    wi.pValues        = &value;
    vkWaitSemaphores(m_device, &wi, timeout);
}

void TimelineSemaphore::signal(uint64_t value)
{
    VkSemaphoreSignalInfo si{VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO};
    si.semaphore = m_semaphore;
    si.value     = value;
    vkSignalSemaphore(m_device, &si);
}

} // namespace fusion
