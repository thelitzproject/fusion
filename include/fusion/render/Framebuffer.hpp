#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace fusion {

class Device;
class RenderPass;

class Framebuffer {
public:
    Framebuffer(const Device& device,
                const RenderPass& renderPass,
                const std::vector<VkImageView>& attachments,
                uint32_t width,
                uint32_t height,
                uint32_t layers = 1);
    ~Framebuffer();

    Framebuffer(const Framebuffer&)            = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    VkFramebuffer handle()  const { return m_framebuffer; }
    uint32_t      width()   const { return m_width; }
    uint32_t      height()  const { return m_height; }

private:
    VkDevice      m_device      = VK_NULL_HANDLE;
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    uint32_t      m_width       = 0;
    uint32_t      m_height      = 0;
};

} // namespace fusion
