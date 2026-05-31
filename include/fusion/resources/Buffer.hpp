#pragma once
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <cstddef>
#include <span>

namespace fusion {

class Allocator;
class Device;

enum class BufferUsage {
    Vertex,
    Index,
    Uniform,
    Storage,
    Staging,
    Indirect,
};

struct BufferConfig {
    size_t      size  = 0;
    BufferUsage usage = BufferUsage::Staging;
    bool        hostVisible = false; // GPU-only by default; set true for persistent mapping
};

class Buffer {
public:
    Buffer(const Allocator& allocator, const BufferConfig& cfg);
    ~Buffer();

    Buffer(const Buffer&)            = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&) noexcept;
    Buffer& operator=(Buffer&&) noexcept;

    VkBuffer     handle() const { return m_buffer; }
    VmaAllocation allocation() const { return m_allocation; }
    size_t        size()   const { return m_size; }

    void* map();
    void  unmap();
    void  upload(const void* data, size_t size, size_t offset = 0);

    // Convenience: upload via staging then transfer to GPU-local
    static void uploadToGPU(const Device& device,
                             VkCommandPool pool,
                             const Allocator& allocator,
                             Buffer& dst,
                             const void* data,
                             size_t size);

private:
    VmaAllocator  m_allocator  = VK_NULL_HANDLE;
    VkBuffer      m_buffer     = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    size_t        m_size       = 0;
    void*         m_mapped     = nullptr;
};

} // namespace fusion
