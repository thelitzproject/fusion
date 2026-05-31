#include <fusion/fusion.hpp>
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <stdexcept>
#include <iostream>

int main()
{
    if (!glfwInit())
        throw std::runtime_error("glfwInit failed");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Fusion — Triangle", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("glfwCreateWindow failed");
    }

    try {
        fusion::ContextConfig cfg;
        cfg.instance.appName = "triangle";

        fusion::Context  ctx(window, cfg);
        fusion::Renderer renderer(ctx, window);

        const auto& dev = ctx.device();

        fusion::PipelineCache pipelineCache(dev, "pipeline.cache");

        fusion::Shader vert(dev, "shaders/spirv/triangle.vert.spv", fusion::ShaderStage::Vertex);
        fusion::Shader frag(dev, "shaders/spirv/triangle.frag.spv", fusion::ShaderStage::Fragment);

        // Hardcoded positions in vertex shader — no vertex input needed
        fusion::PipelineConfig pipelineCfg;
        pipelineCfg.depthTest  = false;
        pipelineCfg.depthWrite = false;

        fusion::Pipeline pipeline(dev, renderer.renderPass(),
            { &vert, &frag }, pipelineCfg, pipelineCache.handle());

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
        pipelineCache.save("pipeline.cache");
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
