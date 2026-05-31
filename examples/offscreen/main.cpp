#include <fusion/fusion.hpp>
#include <GLFW/glfw3.h>

#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>
#include <stdexcept>

static constexpr uint32_t WIDTH  = 512;
static constexpr uint32_t HEIGHT = 512;

// Write raw RGBA pixels as a binary PPM (P6) image.
static void writePPM(const std::string& path,
                     const std::vector<uint8_t>& pixels,
                     uint32_t w, uint32_t h)
{
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) throw std::runtime_error("cannot open output file: " + path);
    f << "P6\n" << w << " " << h << "\n255\n";
    for (uint32_t i = 0; i < w * h; ++i)
        f.write(reinterpret_cast<const char*>(&pixels[i * 4]), 3); // drop alpha
}

int main()
{
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");

    // Hidden window — we only need the Vulkan surface for device selection.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE,    GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "offscreen", nullptr, nullptr);
    if (!window) { glfwTerminate(); throw std::runtime_error("glfwCreateWindow failed"); }

    try {
        fusion::ContextConfig cfg;
        cfg.instance.appName = "offscreen";

        fusion::Context ctx(window, cfg);
        const auto& dev   = ctx.device();
        const auto& alloc = ctx.allocator();

        // ── Offscreen color attachment ────────────────────────────────────────
        fusion::ImageConfig imgCfg{};
        imgCfg.width  = WIDTH;
        imgCfg.height = HEIGHT;
        imgCfg.format = VK_FORMAT_R8G8B8A8_UNORM;
        imgCfg.usage  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        fusion::Image colorImg(alloc, imgCfg);

        // ── RenderPass with COLOR_ATTACHMENT_OPTIMAL final layout ─────────────
        fusion::AttachmentDesc colorAtt{};
        colorAtt.format      = VK_FORMAT_R8G8B8A8_UNORM;
        colorAtt.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAtt.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
        colorAtt.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        fusion::SubpassDesc sp;
        sp.colorAttachmentRefs.push_back(0);

        fusion::RenderPass renderPass(dev, { colorAtt }, { sp });
        fusion::Framebuffer fb(dev, renderPass, { colorImg.view() }, WIDTH, HEIGHT);

        // ── Command infrastructure ────────────────────────────────────────────
        fusion::CommandPool   cmdPool(dev, dev.graphics().familyIndex());
        fusion::CommandBuffer cmd(dev, cmdPool.handle());

        // ── Shaders and pipeline ──────────────────────────────────────────────
        fusion::Shader vert(dev, "shaders/spirv/triangle.vert.spv", fusion::ShaderStage::Vertex);
        fusion::Shader frag(dev, "shaders/spirv/triangle.frag.spv", fusion::ShaderStage::Fragment);

        fusion::PipelineConfig pipeCfg;
        pipeCfg.depthTest  = false;
        pipeCfg.depthWrite = false;
        fusion::Pipeline pipeline(dev, renderPass, { &vert, &frag }, pipeCfg);

        // ── Staging buffer for readback ───────────────────────────────────────
        size_t bufBytes = static_cast<size_t>(WIDTH) * HEIGHT * 4;
        fusion::Buffer staging(alloc, { bufBytes, fusion::BufferUsage::Staging, true });

        // ── Record ────────────────────────────────────────────────────────────
        cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkClearValue clear{};
        clear.color = {{ 0.05f, 0.05f, 0.15f, 1.0f }};
        cmd.beginRenderPass(renderPass, fb, { WIDTH, HEIGHT }, { clear });
        cmd.setViewport(0.f, 0.f, float(WIDTH), float(HEIGHT));
        cmd.setScissor(0, 0, WIDTH, HEIGHT);
        cmd.bindGraphicsPipeline(pipeline);
        cmd.draw(3);
        cmd.endRenderPass();

        // Transition color attachment → transfer source
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image               = colorImg.handle();
        barrier.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        barrier.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT;
        cmd.pipelineBarrier(
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            { barrier });

        // Copy image → staging buffer
        VkBufferImageCopy region{};
        region.imageSubresource  = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.imageExtent       = { WIDTH, HEIGHT, 1 };
        vkCmdCopyImageToBuffer(cmd.handle(), colorImg.handle(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staging.handle(), 1, &region);

        cmd.end();

        // ── Submit and wait ───────────────────────────────────────────────────
        fusion::Fence done(dev);
        VkCommandBuffer cmdH = cmd.handle();
        VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        si.commandBufferCount = 1;
        si.pCommandBuffers    = &cmdH;
        dev.graphics().submit(si, done.handle());
        done.wait();

        // ── Read back and save ────────────────────────────────────────────────
        std::vector<uint8_t> pixels(bufBytes);
        void* ptr = staging.map();
        std::memcpy(pixels.data(), ptr, bufBytes);
        staging.unmap();

        writePPM("offscreen.ppm", pixels, WIDTH, HEIGHT);
        std::cout << "Written offscreen.ppm (" << WIDTH << "x" << HEIGHT << ")\n";

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
