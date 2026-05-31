# Fusion User Guide

This guide walks through the full API, starting from the minimum setup needed
to open a window and ending with the advanced systems (FrameGraph, compute
pipelines, instancing, offscreen rendering).

---

## Table of contents

1. [Setup and initialisation](#1-setup-and-initialisation)
2. [Context](#2-context)
3. [Renderer and the frame loop](#3-renderer-and-the-frame-loop)
4. [Shaders](#4-shaders)
5. [Pipelines](#5-pipelines)
6. [Buffers](#6-buffers)
7. [Images and samplers](#7-images-and-samplers)
8. [Descriptor sets](#8-descriptor-sets)
9. [Command recording](#9-command-recording)
10. [Synchronisation](#10-synchronisation)
11. [FrameGraph](#11-framegraph)
12. [Instanced drawing](#12-instanced-drawing)
13. [Offscreen rendering](#13-offscreen-rendering)
14. [Pipeline cache](#14-pipeline-cache)
15. [Debug markers](#15-debug-markers)
16. [Tools](#16-tools)

---

## 1. Setup and initialisation

Include the umbrella header. It exposes the entire public API.

```cpp
#include <fusion/fusion.hpp>
```

Initialise GLFW before creating a window. Set `GLFW_CLIENT_API` to
`GLFW_NO_API` — Vulkan manages the graphics context itself.

```cpp
glfwInit();
glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
GLFWwindow* window = glfwCreateWindow(1280, 720, "My App", nullptr, nullptr);
```

---

## 2. Context

`Context` owns every core Vulkan object: instance, physical device, logical
device, surface, swapchain, and VMA allocator. Pass a `ContextConfig` to
control instance-level and swapchain settings.

```cpp
fusion::ContextConfig cfg;
cfg.instance.appName    = "my-app";
cfg.instance.appVersion = VK_MAKE_VERSION(1, 0, 0);
cfg.instance.validation = true;            // enable Vulkan validation layers
cfg.swapchain.vsync     = true;            // FIFO present mode
cfg.swapchain.imageCount = 3;              // triple-buffered

fusion::Context ctx(window, cfg);
```

Access individual objects via const references:

```cpp
const fusion::Device&         dev   = ctx.device();
const fusion::PhysicalDevice& phys  = ctx.physDevice();
const fusion::Allocator&      alloc = ctx.allocator();
fusion::Swapchain&            sc    = ctx.swapchain();
```

Call `ctx.onResize(window)` when the window is resized (or let `Renderer`
handle it automatically — see below).

---

## 3. Renderer and the frame loop

`Renderer` manages per-frame resources: one command pool and buffer, two
semaphores, and a fence per frame in flight. It also owns the default render
pass and framebuffers for the swapchain.

```cpp
fusion::Renderer renderer(ctx, window);
```

Register the resize callback so the swapchain is recreated when the window
changes size:

```cpp
glfwSetWindowUserPointer(window, &renderer);
glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int, int) {
    static_cast<fusion::Renderer*>(glfwGetWindowUserPointer(w))->notifyResize();
});
```

The main loop calls `drawFrame`, which acquires a swapchain image, begins the
render pass, invokes your callback, and presents.

```cpp
while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    renderer.drawFrame(window, [&](fusion::CommandBuffer& cmd,
                                   uint32_t imageIndex,
                                   uint32_t frameIndex) {
        // record draw commands here
        cmd.bindGraphicsPipeline(pipeline);
        cmd.draw(3);
    });
}

ctx.device().waitIdle();   // drain GPU before destroying resources
```

`drawFrame` returns `false` when the swapchain was just recreated (e.g. window
resize). You can ignore the return value — the next call will succeed.

### Frame index vs image index

- `frameIndex` — cycles 0…`MAX_FRAMES_IN_FLIGHT - 1` (currently 2). Use this
  to index per-frame uniform buffers or descriptor sets.
- `imageIndex` — the swapchain image acquired this frame. Use this when you
  need to write directly into a specific swapchain framebuffer.

---

## 4. Shaders

Load a compiled SPIR-V file from disk:

```cpp
fusion::Shader vert(dev, "shaders/spirv/my.vert.spv", fusion::ShaderStage::Vertex);
fusion::Shader frag(dev, "shaders/spirv/my.frag.spv", fusion::ShaderStage::Fragment);
```

Or supply the SPIR-V words directly (useful for tests or embedded shaders):

```cpp
std::vector<uint32_t> spirv = /* ... */;
fusion::Shader comp(dev, spirv, fusion::ShaderStage::Compute);
```

Available stages: `Vertex`, `Fragment`, `Compute`, `Geometry`, `TessControl`,
`TessEval`.

---

## 5. Pipelines

### Graphics pipeline

Fill a `PipelineConfig` and pass it along with your shaders and the render
pass the pipeline will be used with.

```cpp
fusion::PipelineConfig cfg;

// Vertex input
cfg.vertexBindings   = {{ 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX }};
cfg.vertexAttributes = {
    { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)   },
    { 1, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, uv)    },
};

// Rasterisation
cfg.cullMode    = VK_CULL_MODE_BACK_BIT;
cfg.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
cfg.polygonMode = VK_POLYGON_MODE_FILL;

// Depth
cfg.depthTest    = true;
cfg.depthWrite   = true;
cfg.depthCompare = VK_COMPARE_OP_LESS;

// Alpha blending (optional)
cfg.blendEnable = true;

// Descriptor layouts and push constants
cfg.setLayouts    = { descLayout.handle() };
cfg.pushConstants = { { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushData) } };

fusion::Pipeline pipeline(dev, renderer.renderPass(),
    { &vert, &frag }, cfg, pipelineCache.handle());
```

`viewport` and `scissor` are dynamic by default — set them via `CommandBuffer`
each frame.

### Compute pipeline

```cpp
fusion::Pipeline compute(dev, computeShader,
    { descLayout.handle() }, /* push constants */ {});
```

---

## 6. Buffers

Create a buffer by specifying its usage and size:

```cpp
// GPU-local vertex buffer
fusion::Buffer vtx(alloc, { sizeof(vertices), fusion::BufferUsage::Vertex });

// CPU-accessible uniform buffer (persistently mapped)
fusion::Buffer ubo(alloc, { sizeof(UBOData), fusion::BufferUsage::Uniform, true });
```

Available usages: `Vertex`, `Index`, `Uniform`, `Storage`, `Staging`,
`Indirect`.

### CPU upload

For `Uniform` and `Staging` buffers (or any buffer created with
`hostVisible = true`):

```cpp
ubo.upload(&data, sizeof(data));   // map → memcpy → flush
```

### GPU upload via staging

For GPU-local vertex/index buffers, use the static helper which creates a
transient staging buffer, copies into it, and transfers to the destination:

```cpp
fusion::Buffer::uploadToGPU(dev, pool.handle(), alloc, vtxBuf,
                             vertices.data(), vertices.size() * sizeof(Vertex));
```

---

## 7. Images and samplers

### Creating an image

```cpp
fusion::ImageConfig cfg{};
cfg.width     = 1024;
cfg.height    = 1024;
cfg.format    = VK_FORMAT_R8G8B8A8_SRGB;
cfg.mipLevels = 10;
cfg.usage     = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
              | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

fusion::Image img(alloc, cfg);
```

The image view is created automatically and accessible via `img.view()`.

### Loading from a file

Decodes a PNG/JPG/BMP/TGA file and optionally generates a full mip chain:

```cpp
fusion::Image tex = fusion::Image::loadFromFile(
    dev, cmdPool.handle(), alloc, "textures/albedo.png",
    /* generateMips = */ true);
```

### Transitioning layouts

```cpp
VkCommandBuffer cmd = dev.beginSingleTimeCommands(pool.handle());
img.transitionLayout(cmd,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
dev.endSingleTimeCommands(cmd, pool.handle(), dev.transfer().handle());
```

### Samplers

```cpp
fusion::SamplerConfig sc{};
sc.magFilter    = VK_FILTER_LINEAR;
sc.minFilter    = VK_FILTER_LINEAR;
sc.mipMode      = VK_SAMPLER_MIPMAP_MODE_LINEAR;
sc.addressMode  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
sc.maxAnisotropy = 16.0f;
sc.maxLod       = static_cast<float>(img.mipLevels());

fusion::Sampler sampler(dev, sc);
```

---

## 8. Descriptor sets

### Layout

```cpp
fusion::DescriptorSetLayout layout(dev, {
    { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_VERTEX_BIT   },
    { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },
});
```

### Pool

```cpp
fusion::DescriptorPool pool(dev, /* maxSets = */ 10, {
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         10 },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
});
```

### Allocate and write

```cpp
fusion::DescriptorSet set(dev, pool.handle(), layout.handle());

set.update({
    { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, fusion::BufferWrite{ &ubo } },
    { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         fusion::ImageWrite{ &tex, &sampler,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL } },
});
```

### Binding

```cpp
cmd.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS,
    pipeline.layout(), 0, { set.handle() });
```

---

## 9. Command recording

All draw/dispatch/copy commands are methods on `CommandBuffer`. Viewport and
scissor are dynamic and must be set each frame.

```cpp
cmd.begin();

// State
cmd.beginRenderPass(rp, fb, extent, clearValues);
cmd.setViewport(0, 0, float(extent.width), float(extent.height));
cmd.setScissor(0, 0, extent.width, extent.height);

// Draw
cmd.bindGraphicsPipeline(pipeline);
cmd.bindVertexBuffers(0, { vtxBuf.handle() });
cmd.bindIndexBuffer(idxBuf.handle(), 0);
cmd.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS,
    pipeline.layout(), 0, { set.handle() });

struct Push { glm::mat4 mvp; } push{ mvpMatrix };
cmd.pushConstants(pipeline.layout(), VK_SHADER_STAGE_VERTEX_BIT,
    0, sizeof(push), &push);

cmd.drawIndexed(indexCount);

cmd.endRenderPass();
cmd.end();
```

### Image barriers

```cpp
VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
b.oldLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
b.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
b.image               = img.handle();
b.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
b.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
b.dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT;

cmd.pipelineBarrier(
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    { b });
```

---

## 10. Synchronisation

### Fence

Use fences to block the CPU until the GPU finishes:

```cpp
fusion::Fence fence(dev);           // unsignaled
fusion::Fence signaled(dev, true);  // pre-signaled (useful as per-frame fence)

// Submit work
dev.graphics().submit(submitInfo, fence.handle());
fence.wait();    // blocks until the GPU reaches this fence
fence.reset();   // ready to use again
```

### Binary semaphore

Used for GPU–GPU synchronisation (e.g. image-available → render-finished):

```cpp
fusion::Semaphore sem(dev);
// pass sem.handle() to vkAcquireNextImageKHR / VkSubmitInfo as usual
```

### Timeline semaphore

Carries a monotonically increasing counter, useful for fine-grained frame
ordering and multi-queue work:

```cpp
fusion::TimelineSemaphore tl(dev, /* initialValue = */ 0);

// Signal value 1 from CPU
tl.signal(1);

// Wait for value 1 from CPU (with timeout)
tl.wait(1);

// Query current counter
uint64_t v = tl.value();
```

---

## 11. FrameGraph

The `FrameGraph` manages transient render targets and automatically inserts
image layout transitions between passes. It culls passes whose outputs are
never read.

### Declare passes

Call `addPass` before the first `execute`. Each pass receives a `PassBuilder`
during setup to declare its resource reads and writes.

```cpp
auto& fg = renderer.frameGraph();

FGTextureDesc gbufDesc{};
gbufDesc.format = VK_FORMAT_R16G16B16A16_SFLOAT;  // width/height 0 = match swapchain

FGResourceHandle gbuf, shadow;

fg.addPass("GBuffer",
    [&](fusion::PassBuilder& b) {
        gbuf   = b.createTexture("gbuffer", gbufDesc);
        shadow = b.readTexture(shadowHandle);   // from a previous pass
    },
    [&](fusion::CommandBuffer& cmd) {
        cmd.bindGraphicsPipeline(gbufPipeline);
        cmd.draw(vertCount);
    }
);

fg.addPass("Lighting",
    [&](fusion::PassBuilder& b) {
        b.readTexture(gbuf);
        b.readTexture(shadow);
    },
    [&](fusion::CommandBuffer& cmd) {
        cmd.bindGraphicsPipeline(lightPipeline);
        cmd.draw(3);
    }
);
```

### Execute

```cpp
renderer.drawFrame(window, [&](fusion::CommandBuffer& cmd, uint32_t, uint32_t) {
    fg.execute(cmd);   // compile (first call only), then execute passes in order
});
```

### Access physical resources

After `compile()` or the first `execute()`:

```cpp
VkImageView view = fg.getView(gbuf);    // bind to a descriptor set
VkImage     img  = fg.getImage(gbuf);
VkBuffer    buf  = fg.getBuffer(handle);
```

### Resize

Call `onResize` when the swapchain extent changes (Renderer does this
automatically via `recreateSwapchain`):

```cpp
fg.onResize(newWidth, newHeight);
```

Textures with `width == 0` and `height == 0` are re-allocated at the new size.

---

## 12. Instanced drawing

Use two vertex bindings — one per-vertex, one per-instance:

```glsl
// instanced.vert
layout(location = 0) in vec2 inPos;     // binding 0, rate = vertex
layout(location = 1) in vec2 inOffset;  // binding 1, rate = instance
layout(location = 2) in vec3 inColor;   // binding 1, rate = instance
```

```cpp
struct InstanceData { glm::vec2 offset; glm::vec3 color; };

fusion::PipelineConfig cfg;
cfg.vertexBindings = {
    { 0, sizeof(glm::vec2),   VK_VERTEX_INPUT_RATE_VERTEX   },
    { 1, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE },
};
cfg.vertexAttributes = {
    { 0, 0, VK_FORMAT_R32G32_SFLOAT,    0 },
    { 1, 1, VK_FORMAT_R32G32_SFLOAT,    0 },
    { 2, 1, VK_FORMAT_R32G32B32_SFLOAT, 8 },
};

// In the draw callback:
cmd.bindVertexBuffers(0, { vtxBuf.handle(), instBuf.handle() });
cmd.draw(vertexCount, instanceCount);
```

See `examples/instancing/main.cpp` for a complete working example.

---

## 13. Offscreen rendering

Create your own `Image`, `RenderPass`, and `Framebuffer`. Use a hidden GLFW
window solely for the Vulkan surface (needed for device selection). Never
create a `Renderer` — record and submit commands manually.

```cpp
// Hidden window for surface creation
glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "", nullptr, nullptr);
fusion::Context ctx(window, cfg);
const auto& dev   = ctx.device();
const auto& alloc = ctx.allocator();

// Colour attachment with TRANSFER_SRC so we can read pixels back
fusion::ImageConfig imgCfg{};
imgCfg.width  = WIDTH;
imgCfg.height = HEIGHT;
imgCfg.format = VK_FORMAT_R8G8B8A8_UNORM;
imgCfg.usage  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
fusion::Image colorImg(alloc, imgCfg);

// Render pass that leaves the image in COLOR_ATTACHMENT_OPTIMAL
fusion::AttachmentDesc att{};
att.format      = VK_FORMAT_R8G8B8A8_UNORM;
att.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
fusion::SubpassDesc sp;
sp.colorAttachmentRefs.push_back(0);
fusion::RenderPass rp(dev, { att }, { sp });

fusion::Framebuffer fb(dev, rp, { colorImg.view() }, WIDTH, HEIGHT);

// Record, render, barrier to TRANSFER_SRC, copy to staging buffer
fusion::CommandPool   pool(dev, dev.graphics().familyIndex());
fusion::CommandBuffer cmd(dev, pool.handle());
cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
// ... render pass, draw calls, barrier ...
cmd.end();

// Submit and wait
fusion::Fence done(dev);
VkCommandBuffer h = cmd.handle();
VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
si.commandBufferCount = 1;
si.pCommandBuffers    = &h;
dev.graphics().submit(si, done.handle());
done.wait();

// Read pixels
void* ptr = staging.map();
std::memcpy(pixels.data(), ptr, bufBytes);
staging.unmap();
```

See `examples/offscreen/main.cpp` for the complete implementation.

---

## 14. Pipeline cache

Load an existing cache from disk on startup and save it on shutdown to
accelerate future pipeline compilations:

```cpp
fusion::PipelineCache cache(dev, "pipeline.cache");   // empty path = start fresh

// ... pass cache.handle() to every Pipeline constructor ...
fusion::Pipeline p(dev, rp, shaders, cfg, cache.handle());

// Save on exit (after waitIdle)
dev.waitIdle();
cache.save("pipeline.cache");
```

---

## 15. Debug markers

When building in Debug mode (`-DCMAKE_BUILD_TYPE=Debug`), the library defines
`FUSION_ENABLE_DEBUG_MARKERS`. `CommandBuffer` exposes three marker helpers
that show up in renderdoc, NSight, and other GPU debuggers:

```cpp
cmd.beginLabel("Shadow pass", 0.8f, 0.5f, 0.2f);
// ... shadow draw calls ...
cmd.insertLabel("Shadow map blit");
// ... blit ...
cmd.endLabel();
```

Set a debug name on any Vulkan object:

```cpp
dev.setDebugName(VK_OBJECT_TYPE_IMAGE,
    reinterpret_cast<uint64_t>(img.handle()), "shadowMap");
```

All calls are no-ops in non-debug builds.

---

## 16. Tools

### shadertool

Inspects compiled SPIR-V binaries. Validates the magic number, then prints the
version, generator, id bound, word count, and entry points:

```
$ shadertool shaders/spirv/triangle.vert.spv
shaders/spirv/triangle.vert.spv:
  SPIR-V 1.0
  generator:  0x00080001
  id bound:   14
  word count: 42 (168 bytes)
    EntryPoint: Vertex "main"
```

Pass multiple files to inspect them in one invocation:

```bash
shadertool shaders/spirv/*.spv
```

### assetconv

Converts any image format supported by stb_image to a simple raw binary: a
4-byte width, 4-byte height, then packed RGBA bytes. Useful for shipping
pre-decoded textures that skip the stb decode step at load time.

```bash
# Print info only
assetconv texture.png

# Convert to raw
assetconv texture.png texture.raw

# Batch convert
for f in textures/*.png; do assetconv "$f" "${f%.png}.raw"; done
```

Raw format layout:

| Offset | Size | Content |
|---|---|---|
| 0 | 4 bytes | Width (uint32, little-endian) |
| 4 | 4 bytes | Height (uint32, little-endian) |
| 8 | w × h × 4 bytes | RGBA pixels, row-major |
