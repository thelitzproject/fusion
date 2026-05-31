#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

namespace fusion {

class Device;
class Allocator;
class CommandBuffer;

// ─── Resource handles ────────────────────────────────────────────────────────

struct FGTextureDesc {
    uint32_t          width    = 0; // 0 = match swapchain
    uint32_t          height   = 0;
    VkFormat          format   = VK_FORMAT_R8G8B8A8_SRGB;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    uint32_t          mipLevels = 1;
    VkImageUsageFlags extraUsage = 0;
};

struct FGBufferDesc {
    size_t             size  = 0;
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
};

struct FGResourceHandle { uint32_t id = UINT32_MAX; };
struct FGPassHandle     { uint32_t id = UINT32_MAX; };

// ─── Pass builder ─────────────────────────────────────────────────────────────

class PassBuilder {
public:
    explicit PassBuilder(class FrameGraph& fg, FGPassHandle pass)
        : m_fg(fg), m_pass(pass) {}

    FGResourceHandle createTexture(const std::string& name, const FGTextureDesc& desc);
    FGResourceHandle createBuffer(const std::string& name, const FGBufferDesc& desc);

    FGResourceHandle readTexture(FGResourceHandle h);
    FGResourceHandle readBuffer(FGResourceHandle h);
    FGResourceHandle writeTexture(FGResourceHandle h);
    FGResourceHandle writeBuffer(FGResourceHandle h);

private:
    FrameGraph&    m_fg;
    FGPassHandle   m_pass;
};

// ─── Frame graph ─────────────────────────────────────────────────────────────

using SetupFn   = std::function<void(PassBuilder&)>;
using ExecuteFn = std::function<void(CommandBuffer&)>;

class FrameGraph {
public:
    FrameGraph(const Device& device, const Allocator& allocator,
               uint32_t swapWidth, uint32_t swapHeight);

    FGPassHandle addPass(const std::string& name, SetupFn setup, ExecuteFn execute);

    void compile();
    void execute(CommandBuffer& cmd);

    void onResize(uint32_t width, uint32_t height);

    // Access physical resources after compilation
    VkImage     getImage(FGResourceHandle h)  const;
    VkImageView getView(FGResourceHandle h)   const;
    VkBuffer    getBuffer(FGResourceHandle h) const;

private:
    struct Resource {
        std::string      name;
        bool             isBuffer = false;
        FGTextureDesc    texDesc{};
        FGBufferDesc     bufDesc{};

        VkImage      image      = VK_NULL_HANDLE;
        VkImageView  view       = VK_NULL_HANDLE;
        VkBuffer     buffer     = VK_NULL_HANDLE;
        void*        allocation = nullptr; // VmaAllocation

        uint32_t     firstWrite = UINT32_MAX;
        uint32_t     lastRead   = 0;

        VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    struct Pass {
        std::string name;
        SetupFn     setup;
        ExecuteFn   execute;
        std::vector<uint32_t> reads;
        std::vector<uint32_t> writes;
        bool        culled = false;
    };

    void cullPasses();
    void allocateResources();
    void insertBarriers(CommandBuffer& cmd, uint32_t passIndex);

    const Device&    m_device;
    const Allocator& m_allocator;
    uint32_t         m_swapWidth;
    uint32_t         m_swapHeight;

    std::vector<Pass>     m_passes;
    std::vector<Resource> m_resources;
    std::vector<uint32_t> m_executionOrder;

    bool m_dirty = true;

    friend class PassBuilder;
};

} // namespace fusion
