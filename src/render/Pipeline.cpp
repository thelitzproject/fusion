#include "fusion/render/Pipeline.hpp"
#include "fusion/render/RenderPass.hpp"
#include "fusion/resources/Shader.hpp"
#include "fusion/core/Device.hpp"
#include <stdexcept>

namespace fusion {

void Pipeline::createLayout(const Device& device,
    const std::vector<VkDescriptorSetLayout>& setLayouts,
    const std::vector<VkPushConstantRange>& pushConstants)
{
    VkPipelineLayoutCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    ci.setLayoutCount         = static_cast<uint32_t>(setLayouts.size());
    ci.pSetLayouts            = setLayouts.data();
    ci.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
    ci.pPushConstantRanges    = pushConstants.data();
    if (vkCreatePipelineLayout(m_device, &ci, nullptr, &m_layout) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create pipeline layout");
}

Pipeline::Pipeline(const Device& device, const RenderPass& renderPass,
    const std::vector<const Shader*>& shaders, const PipelineConfig& cfg,
    VkPipelineCache cache)
    : m_device(device.handle())
{
    createLayout(device, cfg.setLayouts, cfg.pushConstants);

    std::vector<VkPipelineShaderStageCreateInfo> stages;
    for (const auto* s : shaders) stages.push_back(s->stageInfo());

    std::vector<VkVertexInputBindingDescription> bindings;
    for (const auto& b : cfg.vertexBindings)
        bindings.push_back({ b.binding, b.stride, b.rate });

    std::vector<VkVertexInputAttributeDescription> attributes;
    for (const auto& a : cfg.vertexAttributes)
        attributes.push_back({ a.location, a.binding, a.format, a.offset });

    VkPipelineVertexInputStateCreateInfo vis{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vis.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindings.size());
    vis.pVertexBindingDescriptions      = bindings.data();
    vis.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vis.pVertexAttributeDescriptions    = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo ias{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ias.topology = cfg.topology;

    VkPipelineViewportStateCreateInfo vps{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vps.viewportCount = 1;
    vps.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rast{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rast.polygonMode = cfg.polygonMode;
    rast.cullMode    = cfg.cullMode;
    rast.frontFace   = cfg.frontFace;
    rast.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    ds.depthTestEnable  = cfg.depthTest  ? VK_TRUE : VK_FALSE;
    ds.depthWriteEnable = cfg.depthWrite ? VK_TRUE : VK_FALSE;
    ds.depthCompareOp   = cfg.depthCompare;

    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAtt.blendEnable    = cfg.blendEnable ? VK_TRUE : VK_FALSE;
    if (cfg.blendEnable) {
        blendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAtt.colorBlendOp        = VK_BLEND_OP_ADD;
        blendAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAtt.alphaBlendOp        = VK_BLEND_OP_ADD;
    }

    VkPipelineColorBlendStateCreateInfo cbs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cbs.attachmentCount = 1;
    cbs.pAttachments    = &blendAtt;

    VkPipelineDynamicStateCreateInfo dyn{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dyn.dynamicStateCount = static_cast<uint32_t>(cfg.dynamicStates.size());
    dyn.pDynamicStates    = cfg.dynamicStates.data();

    VkGraphicsPipelineCreateInfo gci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gci.stageCount          = static_cast<uint32_t>(stages.size());
    gci.pStages             = stages.data();
    gci.pVertexInputState   = &vis;
    gci.pInputAssemblyState = &ias;
    gci.pViewportState      = &vps;
    gci.pRasterizationState = &rast;
    gci.pMultisampleState   = &ms;
    gci.pDepthStencilState  = &ds;
    gci.pColorBlendState    = &cbs;
    gci.pDynamicState       = &dyn;
    gci.layout              = m_layout;
    gci.renderPass          = renderPass.handle();
    gci.subpass             = cfg.subpass;

    if (vkCreateGraphicsPipelines(m_device, cache, 1, &gci, nullptr, &m_pipeline) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create graphics pipeline");
}

Pipeline::Pipeline(const Device& device, const Shader& shader,
    const std::vector<VkDescriptorSetLayout>& setLayouts,
    const std::vector<VkPushConstantRange>& pushConstants,
    VkPipelineCache cache)
    : m_device(device.handle())
{
    createLayout(device, setLayouts, pushConstants);

    VkComputePipelineCreateInfo ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    ci.stage  = shader.stageInfo();
    ci.layout = m_layout;

    if (vkCreateComputePipelines(m_device, cache, 1, &ci, nullptr, &m_pipeline) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create compute pipeline");
}

Pipeline::~Pipeline()
{
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_layout, nullptr);
}

} // namespace fusion
