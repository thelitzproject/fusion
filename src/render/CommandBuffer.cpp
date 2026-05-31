#include "fusion/render/CommandBuffer.hpp"
#include "fusion/render/RenderPass.hpp"
#include "fusion/render/Framebuffer.hpp"
#include "fusion/render/Pipeline.hpp"
#include "fusion/core/Device.hpp"
#include <stdexcept>

namespace fusion {

// ─── CommandBuffer ────────────────────────────────────────────────────────────

CommandBuffer::CommandBuffer(const Device& device, VkCommandPool pool,
    VkCommandBufferLevel level)
    : m_device(device.handle())
{
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool        = pool;
    ai.level              = level;
    ai.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(m_device, &ai, &m_cmd) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to allocate command buffer");
}

void CommandBuffer::begin(VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = flags;
    vkBeginCommandBuffer(m_cmd, &bi);
}

void CommandBuffer::end()
{
    vkEndCommandBuffer(m_cmd);
}

void CommandBuffer::reset()
{
    vkResetCommandBuffer(m_cmd, 0);
}

void CommandBuffer::beginRenderPass(const RenderPass& rp, const Framebuffer& fb,
    VkExtent2D extent, const std::vector<VkClearValue>& clearValues,
    VkSubpassContents contents)
{
    VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    bi.renderPass               = rp.handle();
    bi.framebuffer              = fb.handle();
    bi.renderArea.offset        = { 0, 0 };
    bi.renderArea.extent        = extent;
    bi.clearValueCount          = static_cast<uint32_t>(clearValues.size());
    bi.pClearValues             = clearValues.data();
    vkCmdBeginRenderPass(m_cmd, &bi, contents);
}

void CommandBuffer::endRenderPass()
{
    vkCmdEndRenderPass(m_cmd);
}

void CommandBuffer::nextSubpass(VkSubpassContents contents)
{
    vkCmdNextSubpass(m_cmd, contents);
}

void CommandBuffer::bindGraphicsPipeline(const Pipeline& p)
{
    vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p.handle());
}

void CommandBuffer::bindComputePipeline(const Pipeline& p)
{
    vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, p.handle());
}

void CommandBuffer::bindDescriptorSets(VkPipelineBindPoint bindPoint,
    VkPipelineLayout layout, uint32_t firstSet,
    const std::vector<VkDescriptorSet>& sets,
    const std::vector<uint32_t>& dynamicOffsets)
{
    vkCmdBindDescriptorSets(m_cmd, bindPoint, layout, firstSet,
        static_cast<uint32_t>(sets.size()), sets.data(),
        static_cast<uint32_t>(dynamicOffsets.size()), dynamicOffsets.data());
}

void CommandBuffer::pushConstants(VkPipelineLayout layout, VkShaderStageFlags stages,
    uint32_t offset, uint32_t size, const void* data)
{
    vkCmdPushConstants(m_cmd, layout, stages, offset, size, data);
}

void CommandBuffer::setViewport(float x, float y, float w, float h, float minD, float maxD)
{
    VkViewport vp{ x, y, w, h, minD, maxD };
    vkCmdSetViewport(m_cmd, 0, 1, &vp);
}

void CommandBuffer::setScissor(int32_t x, int32_t y, uint32_t w, uint32_t h)
{
    VkRect2D sc{ { x, y }, { w, h } };
    vkCmdSetScissor(m_cmd, 0, 1, &sc);
}

void CommandBuffer::bindVertexBuffers(uint32_t first,
    const std::vector<VkBuffer>& buffers, const std::vector<VkDeviceSize>& offsets)
{
    std::vector<VkDeviceSize> offs = offsets.empty()
        ? std::vector<VkDeviceSize>(buffers.size(), 0)
        : offsets;
    vkCmdBindVertexBuffers(m_cmd, first, static_cast<uint32_t>(buffers.size()),
        buffers.data(), offs.data());
}

void CommandBuffer::bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType type)
{
    vkCmdBindIndexBuffer(m_cmd, buffer, offset, type);
}

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount,
    uint32_t firstVertex, uint32_t firstInstance)
{
    vkCmdDraw(m_cmd, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount,
    uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    vkCmdDrawIndexed(m_cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::dispatch(uint32_t x, uint32_t y, uint32_t z)
{
    vkCmdDispatch(m_cmd, x, y, z);
}

void CommandBuffer::pipelineBarrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
    const std::vector<VkImageMemoryBarrier>& imageBarriers,
    const std::vector<VkBufferMemoryBarrier>& bufBarriers,
    const std::vector<VkMemoryBarrier>& memBarriers)
{
    vkCmdPipelineBarrier(m_cmd, srcStage, dstStage, 0,
        static_cast<uint32_t>(memBarriers.size()),   memBarriers.data(),
        static_cast<uint32_t>(bufBarriers.size()),   bufBarriers.data(),
        static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data());
}

void CommandBuffer::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size,
    VkDeviceSize srcOffset, VkDeviceSize dstOffset)
{
    VkBufferCopy region{ srcOffset, dstOffset, size };
    vkCmdCopyBuffer(m_cmd, src, dst, 1, &region);
}

void CommandBuffer::copyBufferToImage(VkBuffer src, VkImage dst,
    VkImageLayout layout, uint32_t width, uint32_t height,
    uint32_t layer, uint32_t mipLevel)
{
    VkBufferImageCopy region{};
    region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, mipLevel, layer, 1 };
    region.imageExtent      = { width, height, 1 };
    vkCmdCopyBufferToImage(m_cmd, src, dst, layout, 1, &region);
}

void CommandBuffer::blitImage(VkImage src, VkImage dst,
    VkImageLayout srcLayout, VkImageLayout dstLayout,
    const VkImageBlit& region, VkFilter filter)
{
    vkCmdBlitImage(m_cmd, src, srcLayout, dst, dstLayout, 1, &region, filter);
}

void CommandBuffer::beginLabel(const char* name, float r, float g, float b) const
{
#ifdef FUSION_ENABLE_DEBUG_MARKERS
    auto fn = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
        vkGetDeviceProcAddr(m_device, "vkCmdBeginDebugUtilsLabelEXT"));
    if (!fn) return;
    VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    label.pLabelName = name;
    label.color[0] = r; label.color[1] = g; label.color[2] = b; label.color[3] = 1.0f;
    fn(m_cmd, &label);
#else
    (void)name; (void)r; (void)g; (void)b;
#endif
}

void CommandBuffer::endLabel() const
{
#ifdef FUSION_ENABLE_DEBUG_MARKERS
    auto fn = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
        vkGetDeviceProcAddr(m_device, "vkCmdEndDebugUtilsLabelEXT"));
    if (fn) fn(m_cmd);
#endif
}

void CommandBuffer::insertLabel(const char* name, float r, float g, float b) const
{
#ifdef FUSION_ENABLE_DEBUG_MARKERS
    auto fn = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(
        vkGetDeviceProcAddr(m_device, "vkCmdInsertDebugUtilsLabelEXT"));
    if (!fn) return;
    VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    label.pLabelName = name;
    label.color[0] = r; label.color[1] = g; label.color[2] = b; label.color[3] = 1.0f;
    fn(m_cmd, &label);
#else
    (void)name; (void)r; (void)g; (void)b;
#endif
}

CommandBuffer::CommandBuffer(CommandBuffer&& o) noexcept
    : m_device(o.m_device), m_cmd(o.m_cmd)
{
    o.m_cmd = VK_NULL_HANDLE;
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& o) noexcept
{
    if (this != &o) {
        m_device = o.m_device;
        m_cmd    = o.m_cmd;
        o.m_cmd  = VK_NULL_HANDLE;
    }
    return *this;
}

} // namespace fusion
