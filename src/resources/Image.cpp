#include "fusion/resources/Image.hpp"
#include "fusion/resources/Allocator.hpp"
#include "fusion/resources/Buffer.hpp"
#include "fusion/core/Device.hpp"
#include <stdexcept>
#include <cmath>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace fusion {

Image::Image(const Allocator& allocator, const ImageConfig& cfg)
    : m_allocator(allocator.handle()), m_format(cfg.format),
      m_width(cfg.width), m_height(cfg.height),
      m_mipLevels(cfg.mipLevels), m_layers(cfg.layers)
{
    VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ici.imageType     = VK_IMAGE_TYPE_2D;
    ici.format        = cfg.format;
    ici.extent        = { cfg.width, cfg.height, 1 };
    ici.mipLevels     = cfg.mipLevels;
    ici.arrayLayers   = cfg.layers;
    ici.samples       = cfg.samples;
    ici.tiling        = cfg.tiling;
    ici.usage         = cfg.usage;
    ici.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo aci{};
    aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VmaAllocationInfo allocInfo{};
    if (vmaCreateImage(m_allocator, &ici, &aci, &m_image, &m_allocation, &allocInfo) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create image");

    // Retrieve the device handle from VMA
    VmaAllocatorInfo ai{};
    vmaGetAllocatorInfo(m_allocator, &ai);
    m_device = ai.device;

    createView(m_device, cfg.aspect);
}

Image::~Image()
{
    if (m_view)  vkDestroyImageView(m_device, m_view, nullptr);
    if (m_image) vmaDestroyImage(m_allocator, m_image, m_allocation);
}

Image::Image(Image&& o) noexcept
    : m_allocator(o.m_allocator), m_device(o.m_device),
      m_image(o.m_image), m_view(o.m_view), m_allocation(o.m_allocation),
      m_format(o.m_format), m_width(o.m_width), m_height(o.m_height),
      m_mipLevels(o.m_mipLevels), m_layers(o.m_layers)
{
    o.m_image = VK_NULL_HANDLE;
    o.m_view  = VK_NULL_HANDLE;
    o.m_allocation = VK_NULL_HANDLE;
}

Image& Image::operator=(Image&& o) noexcept
{
    if (this != &o) {
        if (m_view)  vkDestroyImageView(m_device, m_view, nullptr);
        if (m_image) vmaDestroyImage(m_allocator, m_image, m_allocation);

        m_allocator  = o.m_allocator;
        m_device     = o.m_device;
        m_image      = o.m_image;
        m_view       = o.m_view;
        m_allocation = o.m_allocation;
        m_format     = o.m_format;
        m_width      = o.m_width;
        m_height     = o.m_height;
        m_mipLevels  = o.m_mipLevels;
        m_layers     = o.m_layers;

        o.m_image = VK_NULL_HANDLE;
        o.m_view  = VK_NULL_HANDLE;
        o.m_allocation = VK_NULL_HANDLE;
    }
    return *this;
}

void Image::createView(VkDevice device, VkImageAspectFlags aspect)
{
    VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vci.image    = m_image;
    vci.viewType = m_layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    vci.format   = m_format;
    vci.subresourceRange.aspectMask     = aspect;
    vci.subresourceRange.baseMipLevel   = 0;
    vci.subresourceRange.levelCount     = m_mipLevels;
    vci.subresourceRange.baseArrayLayer = 0;
    vci.subresourceRange.layerCount     = m_layers;

    if (vkCreateImageView(device, &vci, nullptr, &m_view) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create image view");
}

void Image::transitionLayout(VkCommandBuffer cmd,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) const
{
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout           = oldLayout;
    barrier.newLayout           = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = m_image;
    barrier.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, m_mipLevels, 0, m_layers };

    if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

Image Image::loadFromFile(const Device& device, VkCommandPool pool,
    const Allocator& allocator, const std::string& path, bool generateMips)
{
    int w, h, channels;
    stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);
    if (!pixels)
        throw std::runtime_error("fusion: failed to load image: " + path);

    size_t size = static_cast<size_t>(w) * h * 4;
    uint32_t mips = generateMips
        ? static_cast<uint32_t>(std::floor(std::log2(std::max(w, h)))) + 1
        : 1;

    ImageConfig cfg{};
    cfg.width     = static_cast<uint32_t>(w);
    cfg.height    = static_cast<uint32_t>(h);
    cfg.mipLevels = mips;
    cfg.usage     = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT;

    Image img(allocator, cfg);

    Buffer staging(allocator, { size, BufferUsage::Staging, true });
    staging.upload(pixels, size);
    stbi_image_free(pixels);

    VkCommandBuffer cmd = device.beginSingleTimeCommands(pool);

    img.transitionLayout(cmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkBufferImageCopy region{};
    region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    region.imageExtent      = { cfg.width, cfg.height, 1 };
    vkCmdCopyBufferToImage(cmd, staging.handle(), img.handle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    if (generateMips && mips > 1) {
        // Blit mip chain
        int32_t mipW = w, mipH = h;
        for (uint32_t i = 1; i < mips; ++i) {
            VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            b.image               = img.handle();
            b.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
            b.dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT;
            b.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            b.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 1, 0, 1 };
            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &b);

            VkImageBlit blit{};
            blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1 };
            blit.srcOffsets[1]  = { mipW, mipH, 1 };
            blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1 };
            blit.dstOffsets[1]  = { std::max(1, mipW / 2), std::max(1, mipH / 2), 1 };
            vkCmdBlitImage(cmd, img.handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                img.handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            mipW = std::max(1, mipW / 2);
            mipH = std::max(1, mipH / 2);
        }
        // Transition all mips to shader read
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.image               = img.handle();
        b.srcAccessMask       = VK_ACCESS_TRANSFER_READ_BIT;
        b.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
        b.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, mips - 1, 0, 1 };
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &b);

        b.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.subresourceRange.baseMipLevel = mips - 1;
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &b);
    } else {
        img.transitionLayout(cmd,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    device.endSingleTimeCommands(cmd, pool, device.transfer().handle());
    return img;
}

} // namespace fusion
