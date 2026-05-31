#include <fusion/fusion.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector>
#include <cstdlib>
#include <cmath>
#include <stdexcept>
#include <iostream>

struct InstanceData {
    glm::vec2 offset;
    glm::vec3 color;
};

// Simple HSV → RGB conversion (full saturation and value)
static glm::vec3 hsvColor(float hue)
{
    float h = hue * 6.0f;
    float f = h - std::floor(h);
    float q = 1.0f - f;
    int   s = static_cast<int>(h) % 6;
    switch (s) {
    case 0: return { 1.0f, f,    0.0f };
    case 1: return { q,    1.0f, 0.0f };
    case 2: return { 0.0f, 1.0f, f    };
    case 3: return { 0.0f, q,    1.0f };
    case 4: return { f,    0.0f, 1.0f };
    default:return { 1.0f, 0.0f, q    };
    }
}

int main()
{
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Fusion — Instancing", nullptr, nullptr);
    if (!window) { glfwTerminate(); throw std::runtime_error("glfwCreateWindow failed"); }

    try {
        fusion::ContextConfig cfg;
        cfg.instance.appName = "instancing";

        fusion::Context  ctx(window, cfg);
        fusion::Renderer renderer(ctx, window);

        const auto& dev   = ctx.device();
        const auto& alloc = ctx.allocator();

        // Base triangle (equilateral, inscribed in unit circle)
        static const glm::vec2 vertices[3] = {
            {  0.0f,   -1.0f },
            {  0.866f,  0.5f },
            { -0.866f,  0.5f },
        };

        // 5×5 grid of instances with rainbow colors
        static constexpr int GRID = 5;
        std::vector<InstanceData> instances;
        instances.reserve(GRID * GRID);
        for (int row = 0; row < GRID; ++row) {
            for (int col = 0; col < GRID; ++col) {
                float hue = static_cast<float>(row * GRID + col) / float(GRID * GRID);
                instances.push_back({
                    { -0.8f + col * 0.4f, -0.8f + row * 0.4f },
                    hsvColor(hue)
                });
            }
        }

        // Upload vertex data (CPU→GPU via host-visible buffer for simplicity)
        fusion::Buffer vtxBuf(alloc, { sizeof(vertices), fusion::BufferUsage::Vertex, true });
        vtxBuf.upload(vertices, sizeof(vertices));

        size_t instBytes = instances.size() * sizeof(InstanceData);
        fusion::Buffer instBuf(alloc, { instBytes, fusion::BufferUsage::Vertex, true });
        instBuf.upload(instances.data(), instBytes);

        // Shaders — instanced.vert reads per-vertex + per-instance attributes;
        // triangle.frag is reused unchanged.
        fusion::Shader vert(dev, "shaders/spirv/instanced.vert.spv", fusion::ShaderStage::Vertex);
        fusion::Shader frag(dev, "shaders/spirv/triangle.frag.spv",  fusion::ShaderStage::Fragment);

        fusion::PipelineConfig pipeCfg;
        pipeCfg.depthTest  = false;
        pipeCfg.depthWrite = false;
        pipeCfg.vertexBindings = {
            { 0, sizeof(glm::vec2),   VK_VERTEX_INPUT_RATE_VERTEX   },
            { 1, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE },
        };
        pipeCfg.vertexAttributes = {
            { 0, 0, VK_FORMAT_R32G32_SFLOAT,    0 },  // inPos
            { 1, 1, VK_FORMAT_R32G32_SFLOAT,    0 },  // inOffset
            { 2, 1, VK_FORMAT_R32G32B32_SFLOAT, 8 },  // inColor
        };

        fusion::PipelineCache pipelineCache(dev);
        fusion::Pipeline pipeline(dev, renderer.renderPass(),
            { &vert, &frag }, pipeCfg, pipelineCache.handle());

        glfwSetWindowUserPointer(window, &renderer);
        glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int, int) {
            static_cast<fusion::Renderer*>(glfwGetWindowUserPointer(w))->notifyResize();
        });

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            renderer.drawFrame(window, [&](fusion::CommandBuffer& cmd, uint32_t, uint32_t) {
                cmd.bindGraphicsPipeline(pipeline);
                cmd.bindVertexBuffers(0, { vtxBuf.handle(), instBuf.handle() });
                cmd.draw(3, static_cast<uint32_t>(instances.size()));
            });
        }

        dev.waitIdle();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
