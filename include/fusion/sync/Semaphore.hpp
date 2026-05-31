#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>

namespace fusion {

class Device;

class Semaphore {
public:
    explicit Semaphore(const Device& device);
    ~Semaphore();

    Semaphore(const Semaphore&)            = delete;
    Semaphore& operator=(const Semaphore&) = delete;
    Semaphore(Semaphore&&) noexcept;
    Semaphore& operator=(Semaphore&&) noexcept;

    VkSemaphore handle() const { return m_semaphore; }

private:
    VkDevice    m_device    = VK_NULL_HANDLE;
    VkSemaphore m_semaphore = VK_NULL_HANDLE;
};

class TimelineSemaphore {
public:
    TimelineSemaphore(const Device& device, uint64_t initialValue = 0);
    ~TimelineSemaphore();

    TimelineSemaphore(const TimelineSemaphore&)            = delete;
    TimelineSemaphore& operator=(const TimelineSemaphore&) = delete;

    VkSemaphore handle() const { return m_semaphore; }

    uint64_t value() const;
    void     wait(uint64_t value, uint64_t timeout = UINT64_MAX) const;
    void     signal(uint64_t value);

private:
    VkDevice    m_device    = VK_NULL_HANDLE;
    VkSemaphore m_semaphore = VK_NULL_HANDLE;
};

} // namespace fusion
