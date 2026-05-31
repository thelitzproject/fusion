#include "fusion/render/Framebuffer.hpp"
#include "fusion/render/RenderPass.hpp"
#include "fusion/core/Device.hpp"
#include <stdexcept>

namespace fusion {

Framebuffer::Framebuffer(const Device& device, const RenderPass& renderPass,
    const std::vector<VkImageView>& attachments,
    uint32_t width, uint32_t height, uint32_t layers)
    : m_device(device.handle()), m_width(width), m_height(height)
{
    VkFramebufferCreateInfo ci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    ci.renderPass      = renderPass.handle();
    ci.attachmentCount = static_cast<uint32_t>(attachments.size());
    ci.pAttachments    = attachments.data();
    ci.width           = width;
    ci.height          = height;
    ci.layers          = layers;

    if (vkCreateFramebuffer(m_device, &ci, nullptr, &m_framebuffer) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create framebuffer");
}

Framebuffer::~Framebuffer()
{
    vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
}

} // namespace fusion
