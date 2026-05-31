#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <variant>

namespace fusion {

class Device;
class Buffer;
class Image;
class Sampler;

struct DescriptorBinding {
    uint32_t          binding;
    VkDescriptorType  type;
    VkShaderStageFlags stages;
    uint32_t          count = 1;
};

class DescriptorSetLayout {
public:
    DescriptorSetLayout(const Device& device,
                        const std::vector<DescriptorBinding>& bindings);
    ~DescriptorSetLayout();

    DescriptorSetLayout(const DescriptorSetLayout&)            = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

    VkDescriptorSetLayout handle() const { return m_layout; }

private:
    VkDevice              m_device = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
};

class DescriptorPool {
public:
    DescriptorPool(const Device& device, uint32_t maxSets,
                   const std::vector<VkDescriptorPoolSize>& sizes);
    ~DescriptorPool();

    DescriptorPool(const DescriptorPool&)            = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;

    VkDescriptorPool handle() const { return m_pool; }
    void             reset();

private:
    VkDevice         m_device = VK_NULL_HANDLE;
    VkDescriptorPool m_pool   = VK_NULL_HANDLE;
};

struct BufferWrite {
    const Buffer* buffer;
    VkDeviceSize  offset = 0;
    VkDeviceSize  range  = VK_WHOLE_SIZE;
};

struct ImageWrite {
    const Image*   image;
    const Sampler* sampler;
    VkImageLayout  layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
};

struct DescriptorWrite {
    uint32_t          binding;
    VkDescriptorType  type;
    std::variant<BufferWrite, ImageWrite> data;
};

class DescriptorSet {
public:
    DescriptorSet(const Device& device,
                  VkDescriptorPool pool,
                  VkDescriptorSetLayout layout);

    VkDescriptorSet handle() const { return m_set; }

    void update(const std::vector<DescriptorWrite>& writes);

private:
    VkDevice        m_device = VK_NULL_HANDLE;
    VkDescriptorSet m_set    = VK_NULL_HANDLE;
};

} // namespace fusion
