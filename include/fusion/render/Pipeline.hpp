#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

namespace fusion {

class Device;
class Shader;
class RenderPass;

struct VertexAttribute {
    uint32_t location;
    uint32_t binding;
    VkFormat format;
    uint32_t offset;
};

struct VertexBinding {
    uint32_t            binding;
    uint32_t            stride;
    VkVertexInputRate   rate = VK_VERTEX_INPUT_RATE_VERTEX;
};

struct PipelineConfig {
    std::vector<VertexBinding>   vertexBindings;
    std::vector<VertexAttribute> vertexAttributes;

    VkPrimitiveTopology topology          = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPolygonMode       polygonMode       = VK_POLYGON_MODE_FILL;
    VkCullModeFlags     cullMode          = VK_CULL_MODE_BACK_BIT;
    VkFrontFace         frontFace         = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    bool                depthTest         = true;
    bool                depthWrite        = true;
    VkCompareOp         depthCompare      = VK_COMPARE_OP_LESS;
    bool                blendEnable       = false;
    uint32_t            subpass           = 0;

    std::vector<VkDynamicState>       dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
    };
    std::vector<VkDescriptorSetLayout> setLayouts;
    std::vector<VkPushConstantRange>   pushConstants;
};

class Pipeline {
public:
    Pipeline(const Device& device,
             const RenderPass& renderPass,
             const std::vector<const Shader*>& shaders,
             const PipelineConfig& cfg,
             VkPipelineCache cache = VK_NULL_HANDLE);

    // Compute pipeline
    Pipeline(const Device& device, const Shader& shader,
             const std::vector<VkDescriptorSetLayout>& setLayouts,
             const std::vector<VkPushConstantRange>& pushConstants,
             VkPipelineCache cache = VK_NULL_HANDLE);

    ~Pipeline();

    Pipeline(const Pipeline&)            = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    VkPipeline       handle() const { return m_pipeline; }
    VkPipelineLayout layout() const { return m_layout; }

private:
    void createLayout(const Device& device,
                      const std::vector<VkDescriptorSetLayout>& setLayouts,
                      const std::vector<VkPushConstantRange>& pushConstants);

    VkDevice         m_device   = VK_NULL_HANDLE;
    VkPipeline       m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_layout   = VK_NULL_HANDLE;
};

} // namespace fusion
