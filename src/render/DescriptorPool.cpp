#include "fusion/render/DescriptorSet.hpp"
#include "fusion/core/Device.hpp"
#include <stdexcept>

namespace fusion {

DescriptorPool::DescriptorPool(const Device& device, uint32_t maxSets,
    const std::vector<VkDescriptorPoolSize>& sizes)
    : m_device(device.handle())
{
    VkDescriptorPoolCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    ci.maxSets       = maxSets;
    ci.poolSizeCount = static_cast<uint32_t>(sizes.size());
    ci.pPoolSizes    = sizes.data();
    ci.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    if (vkCreateDescriptorPool(m_device, &ci, nullptr, &m_pool) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create descriptor pool");
}

DescriptorPool::~DescriptorPool()
{
    vkDestroyDescriptorPool(m_device, m_pool, nullptr);
}

void DescriptorPool::reset()
{
    vkResetDescriptorPool(m_device, m_pool, 0);
}

} // namespace fusion
