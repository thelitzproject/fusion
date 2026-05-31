# Fusion

A thin, modern C++20 Vulkan rendering library.

Fusion wraps the Vulkan API into RAII types that compose cleanly without hiding
the graphics pipeline from you. It handles device selection, memory allocation
via VMA, swapchain management, and frame synchronisation so you can focus on
what your renderer actually does.

---

## Features

- **Core** — `Instance`, `PhysicalDevice`, `Device`, `Surface`, `Swapchain`
- **Rendering** — `RenderPass`, `Pipeline` (graphics + compute), `PipelineCache`, `Framebuffer`, `CommandBuffer`, `DescriptorSet`, `FrameGraph`
- **Resources** — `Buffer`, `Image`, `Sampler`, `Shader`, GPU memory via [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- **Sync** — `Fence`, `Semaphore`, `TimelineSemaphore`
- **Extras** — debug marker support (`FUSION_ENABLE_DEBUG_MARKERS`), Vulkan validation layer integration, pipeline cache persistence

---

## Requirements

| Dependency | Minimum | Notes |
|---|---|---|
| C++ compiler | C++20 | GCC 12+, Clang 15+, MSVC 19.34+ |
| CMake | 3.20 | |
| Vulkan SDK | 1.2 | Headers + loader |
| GLFW | 3.3 | Window and surface creation |
| GLM | 0.9.9 | Math library |
| glslc | any | Shader compilation (optional — disables shader targets if absent) |

Third-party headers are expected under `third_party/`:

```
third_party/
  VulkanMemoryAllocator/include/vk_mem_alloc.h
  stb/stb_image.h
```

Neither is committed; see [Contributing](CONTRIBUTING.md#dependencies) for setup instructions.

---

## Building

```bash
git clone <repo>
cd fusion

# Populate third-party headers (one-time)
curl -Lo third_party/VulkanMemoryAllocator/include/vk_mem_alloc.h \
  https://raw.githubusercontent.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/master/include/vk_mem_alloc.h
curl -Lo third_party/stb/stb_image.h \
  https://raw.githubusercontent.com/nothings/stb/master/stb_image.h

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

### CMake options

| Option | Default | Description |
|---|---|---|
| `FUSION_BUILD_EXAMPLES` | `ON` | Build example applications |
| `FUSION_BUILD_TESTS` | `ON` | Build unit and integration tests |
| `FUSION_BUILD_BENCH` | `OFF` | Build benchmarks |
| `FUSION_BUILD_TOOLS` | `ON` | Build developer tools (`shadertool`, `assetconv`) |

### Build configurations

- **Debug** — enables Vulkan validation layers (`FUSION_ENABLE_VALIDATION`) and GPU debug markers (`FUSION_ENABLE_DEBUG_MARKERS`)
- **Release** — strips validation and debug overhead

---

## Quick start

```cpp
#include <fusion/fusion.hpp>
#include <GLFW/glfw3.h>

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "My App", nullptr, nullptr);

    fusion::ContextConfig cfg;
    cfg.instance.appName = "my-app";

    fusion::Context  ctx(window, cfg);
    fusion::Renderer renderer(ctx, window);
    const auto& dev = ctx.device();

    fusion::Shader vert(dev, "shaders/spirv/triangle.vert.spv", fusion::ShaderStage::Vertex);
    fusion::Shader frag(dev, "shaders/spirv/triangle.frag.spv", fusion::ShaderStage::Fragment);

    fusion::PipelineConfig pipeCfg;
    pipeCfg.depthTest = false;
    fusion::Pipeline pipeline(dev, renderer.renderPass(), {&vert, &frag}, pipeCfg);

    glfwSetWindowUserPointer(window, &renderer);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int, int) {
        static_cast<fusion::Renderer*>(glfwGetWindowUserPointer(w))->notifyResize();
    });

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderer.drawFrame(window, [&](fusion::CommandBuffer& cmd, uint32_t, uint32_t) {
            cmd.bindGraphicsPipeline(pipeline);
            cmd.draw(3);
        });
    }

    dev.waitIdle();
    glfwDestroyWindow(window);
    glfwTerminate();
}
```

---

## Examples

| Example | Description |
|---|---|
| `triangle` | Minimal coloured triangle with hardcoded vertex positions |
| `instancing` | 5×5 rainbow grid drawn with a single instanced draw call |
| `offscreen` | Renders to an offscreen attachment and saves `offscreen.ppm` |

---

## Tools

| Tool | Usage |
|---|---|
| `shadertool` | Inspect SPIR-V binaries: validates magic, prints version, generator, id bound, and entry points |
| `assetconv` | Load PNG/JPG/BMP/TGA images via stb_image and export raw RGBA binary assets |

```bash
./shadertool shaders/spirv/triangle.vert.spv
./assetconv texture.png texture.raw
```

---

## Tests

```bash
cd build
ctest --output-on-failure
# or run directly:
./tests/unit/unit_tests
./tests/integration/integration_tests   # skips gracefully if no GPU found
```

Integration tests use Vulkan directly (no GLFW) and are safe to run in
headless CI environments. They report `[SKIP]` when no Vulkan-capable GPU is
present.

