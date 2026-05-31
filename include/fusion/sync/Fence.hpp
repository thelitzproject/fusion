#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>

namespace fusion {

class Device;

class Fence {
public:
    explicit Fence(const Device& device, bool signaled = false);
    ~Fence();

    Fence(const Fence&)            = delete;
    Fence& operator=(const Fence&) = delete;
    Fence(Fence&&) noexcept;
    Fence& operator=(Fence&&) noexcept;

    VkFence handle() const { return m_fence; }

    void wait(uint64_t timeout = UINT64_MAX) const;
    void reset();
    bool isSignaled() const;

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkFence  m_fence  = VK_NULL_HANDLE;
};

} // namespace fusion
