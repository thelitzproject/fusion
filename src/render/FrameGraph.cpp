#include "fusion/render/FrameGraph.hpp"
#include "fusion/render/CommandBuffer.hpp"
#include "fusion/core/Device.hpp"
#include "fusion/resources/Allocator.hpp"
#include <stdexcept>
#include <algorithm>

#include <vk_mem_alloc.h>

namespace fusion {

// ─── PassBuilder ─────────────────────────────────────────────────────────────

FGResourceHandle PassBuilder::createTexture(const std::string& name, const FGTextureDesc& desc)
{
    FGResourceHandle h{ static_cast<uint32_t>(m_fg.m_resources.size()) };
    FrameGraph::Resource r{};
    r.name      = name;
    r.isBuffer  = false;
    r.texDesc   = desc;
    r.firstWrite = m_pass.id;
    m_fg.m_resources.push_back(std::move(r));
    m_fg.m_passes[m_pass.id].writes.push_back(h.id);
    return h;
}

FGResourceHandle PassBuilder::createBuffer(const std::string& name, const FGBufferDesc& desc)
{
    FGResourceHandle h{ static_cast<uint32_t>(m_fg.m_resources.size()) };
    FrameGraph::Resource r{};
    r.name     = name;
    r.isBuffer = true;
    r.bufDesc  = desc;
    r.firstWrite = m_pass.id;
    m_fg.m_resources.push_back(std::move(r));
    m_fg.m_passes[m_pass.id].writes.push_back(h.id);
    return h;
}

FGResourceHandle PassBuilder::readTexture(FGResourceHandle h)
{
    m_fg.m_passes[m_pass.id].reads.push_back(h.id);
    m_fg.m_resources[h.id].lastRead = std::max(m_fg.m_resources[h.id].lastRead, m_pass.id);
    return h;
}

FGResourceHandle PassBuilder::readBuffer(FGResourceHandle h)
{
    return readTexture(h); // same bookkeeping
}

FGResourceHandle PassBuilder::writeTexture(FGResourceHandle h)
{
    m_fg.m_passes[m_pass.id].writes.push_back(h.id);
    m_fg.m_resources[h.id].firstWrite = std::min(m_fg.m_resources[h.id].firstWrite, m_pass.id);
    return h;
}

FGResourceHandle PassBuilder::writeBuffer(FGResourceHandle h)
{
    return writeTexture(h);
}

// ─── FrameGraph ───────────────────────────────────────────────────────────────

FrameGraph::FrameGraph(const Device& device, const Allocator& allocator,
    uint32_t swapWidth, uint32_t swapHeight)
    : m_device(device), m_allocator(allocator),
      m_swapWidth(swapWidth), m_swapHeight(swapHeight)
{}

FGPassHandle FrameGraph::addPass(const std::string& name, SetupFn setup, ExecuteFn execute)
{
    FGPassHandle h{ static_cast<uint32_t>(m_passes.size()) };
    m_passes.push_back({ name, std::move(setup), std::move(execute), {}, {}, false });
    PassBuilder builder(*this, h);
    m_passes[h.id].setup(builder);
    m_dirty = true;
    return h;
}

void FrameGraph::compile()
{
    if (!m_dirty) return;

    cullPasses();
    allocateResources();

    m_executionOrder.clear();
    for (uint32_t i = 0; i < m_passes.size(); ++i)
        if (!m_passes[i].culled) m_executionOrder.push_back(i);

    m_dirty = false;
}

void FrameGraph::execute(CommandBuffer& cmd)
{
    if (m_dirty) compile();

    for (uint32_t pi : m_executionOrder) {
        cmd.beginLabel(m_passes[pi].name.c_str());
        insertBarriers(cmd, pi);
        m_passes[pi].execute(cmd);
        cmd.endLabel();
    }
}

void FrameGraph::onResize(uint32_t width, uint32_t height)
{
    m_swapWidth  = width;
    m_swapHeight = height;

    // Free and re-allocate transient resources sized to swapchain
    VmaAllocator vma = m_allocator.handle();
    for (auto& r : m_resources) {
        if (!r.isBuffer && r.texDesc.width == 0) {
            if (r.view)  vkDestroyImageView(m_device.handle(), r.view, nullptr);
            if (r.image) vmaDestroyImage(vma, r.image, static_cast<VmaAllocation>(r.allocation));
            r.view  = VK_NULL_HANDLE;
            r.image = VK_NULL_HANDLE;
            r.allocation = nullptr;
        }
    }
    m_dirty = true;
}

VkImage FrameGraph::getImage(FGResourceHandle h) const
{
    return m_resources[h.id].image;
}

VkImageView FrameGraph::getView(FGResourceHandle h) const
{
    return m_resources[h.id].view;
}

VkBuffer FrameGraph::getBuffer(FGResourceHandle h) const
{
    return m_resources[h.id].buffer;
}

void FrameGraph::cullPasses()
{
    // Simple ref-count cull: passes that write only to resources nobody reads are culled.
    // A production frame graph would do a full topological ref-count pass; this covers
    // the common case of dangling post-process passes.
    std::vector<uint32_t> readCount(m_resources.size(), 0);
    for (const auto& p : m_passes)
        for (uint32_t r : p.reads) readCount[r]++;

    for (auto& p : m_passes) {
        bool needed = false;
        for (uint32_t w : p.writes)
            if (readCount[w] > 0) { needed = true; break; }
        if (p.writes.empty()) needed = true; // pass with no outputs (e.g. present) always runs
        p.culled = !needed;
    }
}

void FrameGraph::allocateResources()
{
    VmaAllocator vma = m_allocator.handle();

    for (auto& r : m_resources) {
        if (r.allocation) continue; // already allocated

        if (r.isBuffer) {
            VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            bci.size  = r.bufDesc.size;
            bci.usage = r.bufDesc.usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo aci{};
            aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            VmaAllocation alloc = VK_NULL_HANDLE;
            vmaCreateBuffer(vma, &bci, &aci, &r.buffer, &alloc, nullptr);
            r.allocation = alloc;
        } else {
            uint32_t w = r.texDesc.width  ? r.texDesc.width  : m_swapWidth;
            uint32_t h = r.texDesc.height ? r.texDesc.height : m_swapHeight;

            VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
            ici.imageType   = VK_IMAGE_TYPE_2D;
            ici.format      = r.texDesc.format;
            ici.extent      = { w, h, 1 };
            ici.mipLevels   = r.texDesc.mipLevels;
            ici.arrayLayers = 1;
            ici.samples     = r.texDesc.samples;
            ici.tiling      = VK_IMAGE_TILING_OPTIMAL;
            ici.usage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                              VK_IMAGE_USAGE_SAMPLED_BIT          |
                              VK_IMAGE_USAGE_TRANSFER_SRC_BIT     |
                              r.texDesc.extraUsage;
            ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            ici.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo aci{};
            aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            VmaAllocation alloc = VK_NULL_HANDLE;
            vmaCreateImage(vma, &ici, &aci, &r.image, &alloc, nullptr);
            r.allocation = alloc;

            VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            vci.image    = r.image;
            vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            vci.format   = r.texDesc.format;
            vci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, r.texDesc.mipLevels, 0, 1 };
            vkCreateImageView(m_device.handle(), &vci, nullptr, &r.view);
        }
    }
}

void FrameGraph::insertBarriers(CommandBuffer& cmd, uint32_t passIndex)
{
    const auto& pass = m_passes[passIndex];

    std::vector<VkImageMemoryBarrier> imageBarriers;

    for (uint32_t ri : pass.reads) {
        auto& r = m_resources[ri];
        if (r.isBuffer || !r.image) continue;

        VkImageLayout targetLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        if (r.currentLayout == targetLayout) continue;

        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.oldLayout           = r.currentLayout;
        b.newLayout           = targetLayout;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image               = r.image;
        b.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, r.texDesc.mipLevels, 0, 1 };
        b.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        b.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
        imageBarriers.push_back(b);
        r.currentLayout = targetLayout;
    }

    for (uint32_t wi : pass.writes) {
        auto& r = m_resources[wi];
        if (r.isBuffer || !r.image) continue;

        VkImageLayout targetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        if (r.currentLayout == targetLayout) continue;

        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.oldLayout           = r.currentLayout;
        b.newLayout           = targetLayout;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image               = r.image;
        b.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, r.texDesc.mipLevels, 0, 1 };
        b.srcAccessMask       = VK_ACCESS_SHADER_READ_BIT;
        b.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        imageBarriers.push_back(b);
        r.currentLayout = targetLayout;
    }

    if (!imageBarriers.empty()) {
        cmd.pipelineBarrier(
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            imageBarriers);
    }
}

} // namespace fusion
