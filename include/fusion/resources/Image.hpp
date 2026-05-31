#pragma once
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <string>

namespace fusion {

class Allocator;
class Device;

struct ImageConfig {
    uint32_t             width       = 1;
    uint32_t             height      = 1;
    uint32_t             mipLevels   = 1;
    uint32_t             layers      = 1;
    VkFormat             format      = VK_FORMAT_R8G8B8A8_SRGB;
    VkImageTiling        tiling      = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags    usage       = VK_IMAGE_USAGE_SAMPLED_BIT;
    VkSampleCountFlagBits samples    = VK_SAMPLE_COUNT_1_BIT;
    VkImageAspectFlags   aspect      = VK_IMAGE_ASPECT_COLOR_BIT;
};

class Image {
public:
    Image(const Allocator& allocator, const ImageConfig& cfg);
    ~Image();

    Image(const Image&)            = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&&) noexcept;
    Image& operator=(Image&&) noexcept;

    VkImage     handle()    const { return m_image; }
    VkImageView view()      const { return m_view; }
    VkFormat    format()    const { return m_format; }
    uint32_t    width()     const { return m_width; }
    uint32_t    height()    const { return m_height; }
    uint32_t    mipLevels() const { return m_mipLevels; }

    void transitionLayout(VkCommandBuffer cmd,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout,
                           VkPipelineStageFlags srcStage  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           VkPipelineStageFlags dstStage  = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) const;

    static Image loadFromFile(const Device& device,
                               VkCommandPool pool,
                               const Allocator& allocator,
                               const std::string& path,
                               bool generateMips = true);

private:
    void createView(VkDevice device, VkImageAspectFlags aspect);

    VmaAllocator  m_allocator  = VK_NULL_HANDLE;
    VkDevice      m_device     = VK_NULL_HANDLE;
    VkImage       m_image      = VK_NULL_HANDLE;
    VkImageView   m_view       = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkFormat      m_format{};
    uint32_t      m_width     = 0;
    uint32_t      m_height    = 0;
    uint32_t      m_mipLevels = 1;
    uint32_t      m_layers    = 1;
};

} // namespace fusion
