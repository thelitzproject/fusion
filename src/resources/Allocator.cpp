#include "fusion/resources/Allocator.hpp"
#include "fusion/core/Instance.hpp"
#include "fusion/core/PhysicalDevice.hpp"
#include "fusion/core/Device.hpp"
#include <stdexcept>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace fusion {

Allocator::Allocator(const Instance& instance,
                     const PhysicalDevice& physDevice,
                     const Device& device)
{
    VmaAllocatorCreateInfo ci{};
    ci.vulkanApiVersion = VK_API_VERSION_1_2;
    ci.instance         = instance.handle();
    ci.physicalDevice   = physDevice.handle();
    ci.device           = device.handle();

    if (vmaCreateAllocator(&ci, &m_allocator) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create VMA allocator");
}

Allocator::~Allocator()
{
    vmaDestroyAllocator(m_allocator);
}

} // namespace fusion
