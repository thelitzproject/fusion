#include "fusion/render/RenderPass.hpp"
#include "fusion/core/Device.hpp"
#include <stdexcept>

namespace fusion {

RenderPass::RenderPass(const Device& device,
    const std::vector<AttachmentDesc>& attachments,
    const std::vector<SubpassDesc>& subpasses)
    : m_device(device.handle())
{
    std::vector<VkAttachmentDescription> vkAttachments;
    for (const auto& a : attachments) {
        VkAttachmentDescription d{};
        d.format         = a.format;
        d.samples        = a.samples;
        d.loadOp         = a.loadOp;
        d.storeOp        = a.storeOp;
        d.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        d.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        d.initialLayout  = a.initialLayout;
        d.finalLayout    = a.finalLayout;
        vkAttachments.push_back(d);
    }

    std::vector<std::vector<VkAttachmentReference>> colorRefs(subpasses.size());
    std::vector<std::optional<VkAttachmentReference>> depthRefs(subpasses.size());
    std::vector<std::vector<VkAttachmentReference>> inputRefs(subpasses.size());
    std::vector<VkSubpassDescription> vkSubpasses;

    for (size_t i = 0; i < subpasses.size(); ++i) {
        for (uint32_t ref : subpasses[i].colorAttachmentRefs) {
            colorRefs[i].push_back({ ref, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
        }
        for (uint32_t ref : subpasses[i].inputAttachmentRefs) {
            inputRefs[i].push_back({ ref, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
        }
        if (subpasses[i].depthAttachmentRef) {
            depthRefs[i] = { *subpasses[i].depthAttachmentRef,
                             VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
        }

        VkSubpassDescription sp{};
        sp.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sp.colorAttachmentCount    = static_cast<uint32_t>(colorRefs[i].size());
        sp.pColorAttachments       = colorRefs[i].data();
        sp.pDepthStencilAttachment = depthRefs[i] ? &depthRefs[i].value() : nullptr;
        sp.inputAttachmentCount    = static_cast<uint32_t>(inputRefs[i].size());
        sp.pInputAttachments       = inputRefs[i].data();
        vkSubpasses.push_back(sp);
    }

    VkSubpassDependency dep{};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo ci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    ci.attachmentCount = static_cast<uint32_t>(vkAttachments.size());
    ci.pAttachments    = vkAttachments.data();
    ci.subpassCount    = static_cast<uint32_t>(vkSubpasses.size());
    ci.pSubpasses      = vkSubpasses.data();
    ci.dependencyCount = 1;
    ci.pDependencies   = &dep;

    if (vkCreateRenderPass(m_device, &ci, nullptr, &m_renderPass) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create render pass");
}

RenderPass::~RenderPass()
{
    if (m_renderPass) vkDestroyRenderPass(m_device, m_renderPass, nullptr);
}

RenderPass::RenderPass(RenderPass&& o) noexcept
    : m_device(o.m_device), m_renderPass(o.m_renderPass)
{
    o.m_renderPass = VK_NULL_HANDLE;
}

RenderPass& RenderPass::operator=(RenderPass&& o) noexcept
{
    if (this != &o) {
        if (m_renderPass) vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        m_device     = o.m_device;
        m_renderPass = o.m_renderPass;
        o.m_renderPass = VK_NULL_HANDLE;
    }
    return *this;
}

RenderPass RenderPass::makeDefault(const Device& device,
    VkFormat colorFormat, VkFormat depthFormat)
{
    std::vector<AttachmentDesc> attachments;
    attachments.push_back({ colorFormat });

    SubpassDesc sp;
    sp.colorAttachmentRefs.push_back(0);

    if (depthFormat != VK_FORMAT_UNDEFINED) {
        AttachmentDesc depth{};
        depth.format        = depthFormat;
        depth.loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.storeOp       = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth.isDepth       = true;
        attachments.push_back(depth);
        sp.depthAttachmentRef = 1;
    }

    return RenderPass(device, attachments, { sp });
}

} // namespace fusion
