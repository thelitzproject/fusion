#pragma once
#include "Context.hpp"
#include "render/RenderPass.hpp"
#include "render/Framebuffer.hpp"
#include "render/CommandBuffer.hpp"
#include "render/FrameGraph.hpp"
#include "sync/Fence.hpp"
#include "sync/Semaphore.hpp"
#include <vector>
#include <memory>
#include <functional>

namespace fusion {

static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

struct FrameData {
    CommandPool   pool;
    CommandBuffer cmd;
    Semaphore     imageAvailable;
    Semaphore     renderFinished;
    Fence         inFlight;
};

using RenderCallback = std::function<void(CommandBuffer& cmd, uint32_t imageIndex, uint32_t frameIndex)>;

class Renderer {
public:
    Renderer(Context& ctx, GLFWwindow* window);
    ~Renderer() = default;

    // Register passes on the frame graph before first drawFrame()
    FrameGraph&       frameGraph() { return *m_frameGraph; }
    const RenderPass& renderPass() const { return *m_renderPass; }

    // Draw one frame; callback is called between render pass begin/end.
    // Returns false when window was resized and the next call should retry.
    bool drawFrame(GLFWwindow* window, RenderCallback cb);

    void notifyResize() { m_resized = true; }

    uint32_t currentFrame() const { return m_currentFrame; }

private:
    void createFramebuffers();
    void recreateSwapchain(GLFWwindow* window);

    Context& m_ctx;

    std::unique_ptr<RenderPass>             m_renderPass;
    std::vector<std::unique_ptr<Framebuffer>> m_framebuffers;
    std::unique_ptr<FrameGraph>             m_frameGraph;

    std::vector<FrameData> m_frames;
    uint32_t               m_currentFrame = 0;
    bool                   m_resized      = false;
};

} // namespace fusion
