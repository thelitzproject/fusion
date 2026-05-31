#pragma once
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>

namespace fusion {

class Device;

struct AttachmentDesc {
    VkFormat              format      = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits samples     = VK_SAMPLE_COUNT_1_BIT;
    VkAttachmentLoadOp    loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp   storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    VkImageLayout         initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout         finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    bool                  isDepth       = false;
};

struct SubpassDesc {
    std::vector<uint32_t> colorAttachmentRefs;
    std::optional<uint32_t> depthAttachmentRef;
    std::vector<uint32_t> inputAttachmentRefs;
};

class RenderPass {
public:
    RenderPass(const Device& device,
               const std::vector<AttachmentDesc>& attachments,
               const std::vector<SubpassDesc>& subpasses);
    ~RenderPass();

    RenderPass(const RenderPass&)            = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass(RenderPass&&) noexcept;
    RenderPass& operator=(RenderPass&&) noexcept;

    VkRenderPass handle() const { return m_renderPass; }

    // Convenience: single-subpass color + optional depth
    static RenderPass makeDefault(const Device& device,
                                  VkFormat colorFormat,
                                  VkFormat depthFormat = VK_FORMAT_UNDEFINED);

private:
    VkDevice     m_device     = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
};

} // namespace fusion
