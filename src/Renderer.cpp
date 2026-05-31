#include "fusion/Renderer.hpp"
#include <stdexcept>
#include <array>

namespace fusion {

Renderer::Renderer(Context& ctx, GLFWwindow* window)
    : m_ctx(ctx)
{
    const auto& sc = ctx.swapchain();

    m_renderPass = std::make_unique<RenderPass>(RenderPass::makeDefault(
        ctx.device(), sc.format(),
        ctx.physDevice().findDepthFormat()));

    m_frameGraph = std::make_unique<FrameGraph>(
        ctx.device(), ctx.allocator(),
        sc.extent().width, sc.extent().height);

    createFramebuffers();

    const auto& dev = ctx.device();
    uint32_t gfxFamily = dev.graphics().familyIndex();

    m_frames.reserve(MAX_FRAMES_IN_FLIGHT);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_frames.emplace_back(FrameData{
            CommandPool(dev, gfxFamily),
            CommandBuffer(dev, m_frames.back().pool.handle()),  // placeholder — rebuilt below
            Semaphore(dev),
            Semaphore(dev),
            Fence(dev, true)
        });
        // Reallocate with correct pool (the emplace_back above is a workaround
        // for the initializer calling back to back ctors with a moved pool)
    }

    // Rebuild cleanly
    m_frames.clear();
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        CommandPool pool(dev, gfxFamily);
        VkCommandPool rawPool = pool.handle();
        m_frames.push_back({
            std::move(pool),
            CommandBuffer(dev, rawPool),
            Semaphore(dev),
            Semaphore(dev),
            Fence(dev, true)
        });
    }
}

void Renderer::createFramebuffers()
{
    const auto& sc  = m_ctx.swapchain();
    m_framebuffers.clear();
    m_framebuffers.reserve(sc.imageCount());
    for (uint32_t i = 0; i < sc.imageCount(); ++i) {
        m_framebuffers.push_back(std::make_unique<Framebuffer>(
            m_ctx.device(), *m_renderPass,
            std::vector<VkImageView>{ sc.imageView(i) },
            sc.extent().width, sc.extent().height));
    }
}

void Renderer::recreateSwapchain(GLFWwindow* window)
{
    m_ctx.onResize(window);
    createFramebuffers();
    m_frameGraph->onResize(m_ctx.swapchain().extent().width,
                            m_ctx.swapchain().extent().height);
}

bool Renderer::drawFrame(GLFWwindow* window, RenderCallback cb)
{
    auto& frame = m_frames[m_currentFrame];
    frame.inFlight.wait();

    uint32_t imageIndex = 0;
    VkResult result = m_ctx.swapchain().acquireNextImage(
        frame.imageAvailable.handle(), imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain(window);
        return false;
    }

    frame.inFlight.reset();
    frame.cmd.reset();
    frame.cmd.begin();

    const auto& sc = m_ctx.swapchain();
    VkExtent2D extent = sc.extent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color        = {{ 0.01f, 0.01f, 0.01f, 1.0f }};
    clearValues[1].depthStencil = { 1.0f, 0 };

    frame.cmd.beginRenderPass(*m_renderPass, *m_framebuffers[imageIndex], extent,
        { clearValues[0], clearValues[1] });

    frame.cmd.setViewport(0, 0, static_cast<float>(extent.width),
        static_cast<float>(extent.height));
    frame.cmd.setScissor(0, 0, extent.width, extent.height);

    cb(frame.cmd, imageIndex, m_currentFrame);

    frame.cmd.endRenderPass();
    frame.cmd.end();

    VkSemaphore waitSems[]   = { frame.imageAvailable.handle() };
    VkSemaphore signalSems[] = { frame.renderFinished.handle() };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkCommandBuffer cmdHandle = frame.cmd.handle();
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.waitSemaphoreCount   = 1;
    si.pWaitSemaphores      = waitSems;
    si.pWaitDstStageMask    = waitStages;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &cmdHandle;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores    = signalSems;

    m_ctx.device().graphics().submit(si, frame.inFlight.handle());

    result = m_ctx.swapchain().present(frame.renderFinished.handle(), imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_resized) {
        m_resized = false;
        recreateSwapchain(window);
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    return true;
}

} // namespace fusion
