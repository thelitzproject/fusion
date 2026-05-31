#include "fusion/resources/Buffer.hpp"
#include "fusion/resources/Allocator.hpp"
#include "fusion/core/Device.hpp"
#include <stdexcept>
#include <cstring>

namespace fusion {

static VkBufferUsageFlags toVkUsage(BufferUsage u)
{
    switch (u) {
    case BufferUsage::Vertex:   return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT   | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    case BufferUsage::Index:    return VK_BUFFER_USAGE_INDEX_BUFFER_BIT    | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    case BufferUsage::Uniform:  return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    case BufferUsage::Storage:  return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    case BufferUsage::Staging:  return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    case BufferUsage::Indirect: return VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    return 0;
}

Buffer::Buffer(const Allocator& allocator, const BufferConfig& cfg)
    : m_allocator(allocator.handle()), m_size(cfg.size)
{
    VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bci.size        = cfg.size;
    bci.usage       = toVkUsage(cfg.usage);
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo aci{};
    if (cfg.hostVisible || cfg.usage == BufferUsage::Staging || cfg.usage == BufferUsage::Uniform) {
        aci.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        aci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    } else {
        aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    }

    if (vmaCreateBuffer(m_allocator, &bci, &aci, &m_buffer, &m_allocation, nullptr) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create buffer");
}

Buffer::~Buffer()
{
    if (m_mapped) vmaUnmapMemory(m_allocator, m_allocation);
    if (m_buffer) vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
}

Buffer::Buffer(Buffer&& o) noexcept
    : m_allocator(o.m_allocator), m_buffer(o.m_buffer),
      m_allocation(o.m_allocation), m_size(o.m_size), m_mapped(o.m_mapped)
{
    o.m_buffer = VK_NULL_HANDLE;
    o.m_allocation = VK_NULL_HANDLE;
    o.m_mapped = nullptr;
}

Buffer& Buffer::operator=(Buffer&& o) noexcept
{
    if (this != &o) {
        if (m_mapped) vmaUnmapMemory(m_allocator, m_allocation);
        if (m_buffer) vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
        m_allocator  = o.m_allocator;
        m_buffer     = o.m_buffer;
        m_allocation = o.m_allocation;
        m_size       = o.m_size;
        m_mapped     = o.m_mapped;
        o.m_buffer     = VK_NULL_HANDLE;
        o.m_allocation = VK_NULL_HANDLE;
        o.m_mapped     = nullptr;
    }
    return *this;
}

void* Buffer::map()
{
    if (!m_mapped)
        vmaMapMemory(m_allocator, m_allocation, &m_mapped);
    return m_mapped;
}

void Buffer::unmap()
{
    if (m_mapped) {
        vmaUnmapMemory(m_allocator, m_allocation);
        m_mapped = nullptr;
    }
}

void Buffer::upload(const void* data, size_t size, size_t offset)
{
    void* ptr = map();
    std::memcpy(static_cast<char*>(ptr) + offset, data, size);
    vmaFlushAllocation(m_allocator, m_allocation, offset, size);
}

void Buffer::uploadToGPU(const Device& device, VkCommandPool pool,
                          const Allocator& allocator, Buffer& dst,
                          const void* data, size_t size)
{
    Buffer staging(allocator, { size, BufferUsage::Staging, true });
    staging.upload(data, size);

    VkCommandBuffer cmd = device.beginSingleTimeCommands(pool);
    VkBufferCopy region{ 0, 0, size };
    vkCmdCopyBuffer(cmd, staging.handle(), dst.handle(), 1, &region);
    device.endSingleTimeCommands(cmd, pool, device.transfer().handle());
}

} // namespace fusion
