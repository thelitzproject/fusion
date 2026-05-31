#pragma once
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace fusion {

class Instance;
class Device;
class PhysicalDevice;

class Allocator {
public:
    Allocator(const Instance& instance,
              const PhysicalDevice& physDevice,
              const Device& device);
    ~Allocator();

    Allocator(const Allocator&)            = delete;
    Allocator& operator=(const Allocator&) = delete;

    VmaAllocator handle() const { return m_allocator; }

private:
    VmaAllocator m_allocator = VK_NULL_HANDLE;
};

} // namespace fusion
