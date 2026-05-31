# Contributing to Fusion

Thank you for your interest in contributing. This document covers how to get a
working development environment, the conventions the codebase follows, and the
process for submitting changes.

---

## Table of contents

1. [Getting started](#getting-started)
2. [Dependencies](#dependencies)
3. [Building for development](#building-for-development)
4. [Code conventions](#code-conventions)
5. [Adding a feature](#adding-a-feature)
6. [Writing tests](#writing-tests)
7. [Adding an example](#adding-an-example)
8. [Adding a shader](#adding-a-shader)
9. [Submitting a pull request](#submitting-a-pull-request)

---

## Getting started

Fork the repository, clone your fork, and create a branch:

```bash
git clone https://github.com/you/fusion
cd fusion
git checkout -b feature/my-feature
```

---

## Dependencies

### System packages

| Package | Debian/Ubuntu | Fedora |
|---|---|---|
| Vulkan loader + headers | `libvulkan-dev` | `vulkan-loader-devel` |
| GLFW | `libglfw3-dev` | `glfw-devel` |
| GLM | `libglm-dev` | `glm-devel` |
| glslc (shader compiler) | `shaderc` | `shaderc` |
| CMake | `cmake` | `cmake` |

```bash
# Debian/Ubuntu
sudo apt install libvulkan-dev libglfw3-dev libglm-dev shaderc cmake build-essential

# Fedora
sudo dnf install vulkan-loader-devel glfw-devel glm-devel shaderc cmake gcc-c++
```

### Third-party headers

Two header-only libraries live in `third_party/` and are **not committed** to
the repository. Download them once after cloning:

```bash
# VulkanMemoryAllocator
curl -Lo third_party/VulkanMemoryAllocator/include/vk_mem_alloc.h \
  https://raw.githubusercontent.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/master/include/vk_mem_alloc.h

# stb_image (used by Image::loadFromFile and assetconv)
curl -Lo third_party/stb/stb_image.h \
  https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
```

---

## Building for development

Always build in Debug mode during development — it enables Vulkan validation
layers and GPU debug markers:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DFUSION_BUILD_BENCH=ON
cmake --build . -- -j$(nproc)
```

Run the tests after every non-trivial change:

```bash
ctest --output-on-failure
```

---

## Code conventions

### Language and standard

- C++20 throughout. Use `std::optional`, `std::span`, structured bindings, and
  ranges freely.
- No exceptions across public API boundaries — callers receive `VkResult` or
  `std::optional` where possible. Internal helpers throw `std::runtime_error`
  on Vulkan errors.

### Naming

| Kind | Convention | Example |
|---|---|---|
| Types / classes | `PascalCase` | `CommandBuffer`, `FrameGraph` |
| Member variables | `m_camelCase` | `m_device`, `m_renderPass` |
| Free functions | `camelCase` | `chooseFormat()` |
| Constants / macros | `UPPER_SNAKE` | `MAX_FRAMES_IN_FLIGHT` |
| File names | Match the class name | `CommandBuffer.hpp` / `.cpp` |

### File structure

Every class lives in a matching header under `include/fusion/` and a `.cpp`
under `src/`. The directory mirrors the namespace subdivision:

```
include/fusion/core/Device.hpp   →   src/core/Device.cpp
```

Headers use `#pragma once`. Implementation files include their own header
first.

### RAII and ownership

- Every Vulkan object is owned by exactly one RAII wrapper.
- Copy constructors are **deleted** on all wrappers. Move constructors are
  **required** — implement them explicitly because deleting the copy also
  deletes the implicit move.
- Destructors must guard against `VK_NULL_HANDLE` (e.g. `if (m_fence)
  vkDestroyFence(...)`).

```cpp
// Correct pattern for a new wrapper
class MyObject {
public:
    explicit MyObject(const Device& device);
    ~MyObject() { if (m_handle) vkDestroyXxx(m_device, m_handle, nullptr); }

    MyObject(const MyObject&)            = delete;
    MyObject& operator=(const MyObject&) = delete;
    MyObject(MyObject&& o) noexcept : m_device(o.m_device), m_handle(o.m_handle)
        { o.m_handle = VK_NULL_HANDLE; }
    MyObject& operator=(MyObject&& o) noexcept { /* swap-and-null */ return *this; }

    VkXxx handle() const { return m_handle; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkXxx    m_handle = VK_NULL_HANDLE;
};
```

### VMA

`VMA_IMPLEMENTATION` must be defined in **exactly one** translation unit
(`src/resources/Allocator.cpp`). Do not add it anywhere else or to CMake
compile definitions.

### Comments

Write comments only when the *why* is non-obvious. Do not restate what the
code visibly does. Do not write multi-line docstrings.

---

## Adding a feature

1. Add the public declaration to the appropriate header under `include/fusion/`.
2. Implement in the matching `.cpp` under `src/`.
3. Include the header from `include/fusion/fusion.hpp` (the umbrella include)
   if it belongs in the public API.
4. Add the `.cpp` to the source list in the root `CMakeLists.txt`.
5. Cover the new behaviour in either a unit test or an integration test (see
   below).

---

## Writing tests

### Unit tests (`tests/unit/`)

Use for behaviour that does **not** require a GPU (file I/O, pure-CPU
algorithms, data structure invariants). Add a `.cpp` to
`tests/unit/CMakeLists.txt` as a new source or extend `test_pipeline_cache.cpp`.

### Integration tests (`tests/integration/`)

Use for GPU-path behaviour. The integration test binary links against Vulkan
and VMA **directly** (not the fusion library) so it runs safely in headless CI
environments without crashing on GLFW initialisation.

Pattern:

```cpp
static void testMyFeature()
{
    VkInstance inst     = createInstance();
    VkPhysicalDevice ph = pickPhysicalDevice(inst);
    uint32_t family     = 0;
    VkDevice dev        = createDevice(ph, family);

    // ... exercise Vulkan APIs ...

    vkDestroyDevice(dev, nullptr);
    vkDestroyInstance(inst, nullptr);
    std::cout << "[PASS] my feature\n";
}
```

Call your test function from `main()` after the device-availability probe.

### Benchmarks (`bench/`)

Add a new `.cpp` to `bench/` and register it in `bench/CMakeLists.txt`.
Benchmarks are CPU-only (file I/O, data-structure operations). GPU benchmarks
require a real device and are tracked separately.

Enable the bench target with `-DFUSION_BUILD_BENCH=ON`.

---

## Adding an example

1. Create `examples/<name>/main.cpp` and `examples/<name>/CMakeLists.txt`.
2. Use the standard template (see `examples/triangle/CMakeLists.txt`) — link
   against `fusion`, declare an `add_dependencies(... shaders)` and the
   post-build SPIRV copy command.
3. Uncomment (or add) `add_subdirectory(<name>)` in `examples/CMakeLists.txt`.

---

## Adding a shader

1. Place the GLSL source under `shaders/glsl/` with a `.vert`, `.frag`,
   `.comp`, or `.geom` extension.
2. The `shaders/CMakeLists.txt` globs all files in that directory and compiles
   them with `glslc` automatically. No CMake changes are needed.
3. The compiled `.spv` files appear under `shaders/spirv/` and are copied
   next to each example binary by its post-build command.

---

## Submitting a pull request

1. Keep each PR focused on a single concern. Split unrelated cleanups into
   separate PRs.
2. All tests must pass: `ctest --output-on-failure`.
3. New public API must be accompanied by at least one test.
4. Commit messages should describe *why*, not just what:
   ```
   render: add move constructor to RenderPass

   Deleting the copy constructor also suppresses the implicit move, so
   make_unique<RenderPass>(makeDefault(...)) failed to compile.
   ```
5. Open the PR against `main`. The description should list what changed and
   how to test it manually.
