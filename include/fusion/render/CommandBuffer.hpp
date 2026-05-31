#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <span>
#include <vector>

namespace fusion {

class Device;
class RenderPass;
class Framebuffer;
class Pipeline;
class Buffer;

class CommandPool {
public:
    CommandPool(const Device& device, uint32_t queueFamily,
                VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    ~CommandPool();

    CommandPool(const CommandPool&)            = delete;
    CommandPool& operator=(const CommandPool&) = delete;
    CommandPool(CommandPool&&) noexcept;
    CommandPool& operator=(CommandPool&&) noexcept;

    VkCommandPool handle() const { return m_pool; }
    void          reset(VkCommandPoolResetFlags flags = 0);

private:
    VkDevice      m_device = VK_NULL_HANDLE;
    VkCommandPool m_pool   = VK_NULL_HANDLE;
};

class CommandBuffer {
public:
    CommandBuffer(const Device& device, VkCommandPool pool,
                  VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    ~CommandBuffer() = default; // freed with pool

    CommandBuffer(const CommandBuffer&)            = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;
    CommandBuffer(CommandBuffer&&) noexcept;
    CommandBuffer& operator=(CommandBuffer&&) noexcept;

    VkCommandBuffer handle() const { return m_cmd; }

    void begin(VkCommandBufferUsageFlags flags = 0);
    void end();
    void reset();

    // Render pass
    void beginRenderPass(const RenderPass& rp, const Framebuffer& fb,
                         VkExtent2D extent,
                         const std::vector<VkClearValue>& clearValues,
                         VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);
    void endRenderPass();
    void nextSubpass(VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);

    // Pipeline
    void bindGraphicsPipeline(const Pipeline& pipeline);
    void bindComputePipeline(const Pipeline& pipeline);
    void bindDescriptorSets(VkPipelineBindPoint bindPoint,
                             VkPipelineLayout layout,
                             uint32_t firstSet,
                             const std::vector<VkDescriptorSet>& sets,
                             const std::vector<uint32_t>& dynamicOffsets = {});

    void pushConstants(VkPipelineLayout layout, VkShaderStageFlags stages,
                       uint32_t offset, uint32_t size, const void* data);

    // Draw
    void setViewport(float x, float y, float w, float h, float minDepth = 0.f, float maxDepth = 1.f);
    void setScissor(int32_t x, int32_t y, uint32_t w, uint32_t h);
    void bindVertexBuffers(uint32_t first, const std::vector<VkBuffer>& buffers,
                           const std::vector<VkDeviceSize>& offsets = {});
    void bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType type = VK_INDEX_TYPE_UINT32);
    void draw(uint32_t vertexCount, uint32_t instanceCount = 1,
              uint32_t firstVertex = 0, uint32_t firstInstance = 0);
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1,
                     uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                     uint32_t firstInstance = 0);

    // Compute
    void dispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1);

    // Sync
    void pipelineBarrier(VkPipelineStageFlags srcStage,
                          VkPipelineStageFlags dstStage,
                          const std::vector<VkImageMemoryBarrier>& imageBarriers = {},
                          const std::vector<VkBufferMemoryBarrier>& bufBarriers  = {},
                          const std::vector<VkMemoryBarrier>& memBarriers        = {});

    // Copy
    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size,
                    VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);
    void copyBufferToImage(VkBuffer src, VkImage dst,
                            VkImageLayout layout, uint32_t width, uint32_t height,
                            uint32_t layer = 0, uint32_t mipLevel = 0);
    void blitImage(VkImage src, VkImage dst,
                   VkImageLayout srcLayout, VkImageLayout dstLayout,
                   const VkImageBlit& region, VkFilter filter = VK_FILTER_LINEAR);

    // Debug labels (no-op outside debug builds)
    void beginLabel(const char* name, float r = 1, float g = 1, float b = 1) const;
    void endLabel() const;
    void insertLabel(const char* name, float r = 1, float g = 1, float b = 1) const;

private:
    VkDevice        m_device = VK_NULL_HANDLE;
    VkCommandBuffer m_cmd    = VK_NULL_HANDLE;
};

} // namespace fusion
