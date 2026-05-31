#include "fusion/resources/Shader.hpp"
#include "fusion/core/Device.hpp"
#include <fstream>
#include <stdexcept>

namespace fusion {

static std::vector<uint32_t> readSpirv(const std::string& path)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open())
        throw std::runtime_error("fusion: cannot open shader: " + path);
    size_t size = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint32_t> buf(size / 4);
    f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(size));
    return buf;
}

Shader::Shader(const Device& device, const std::string& spirvPath, ShaderStage stage)
    : m_stage(stage)
{
    create(device, readSpirv(spirvPath));
}

Shader::Shader(const Device& device, const std::vector<uint32_t>& spirv, ShaderStage stage)
    : m_stage(stage)
{
    create(device, spirv);
}

Shader::~Shader()
{
    vkDestroyShaderModule(m_device, m_module, nullptr);
}

void Shader::create(const Device& device, const std::vector<uint32_t>& spirv)
{
    m_device = device.handle();
    VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    ci.codeSize = spirv.size() * sizeof(uint32_t);
    ci.pCode    = spirv.data();
    if (vkCreateShaderModule(m_device, &ci, nullptr, &m_module) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create shader module");
}

VkShaderStageFlagBits Shader::vkStage() const
{
    switch (m_stage) {
    case ShaderStage::Vertex:      return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::Fragment:    return VK_SHADER_STAGE_FRAGMENT_BIT;
    case ShaderStage::Compute:     return VK_SHADER_STAGE_COMPUTE_BIT;
    case ShaderStage::Geometry:    return VK_SHADER_STAGE_GEOMETRY_BIT;
    case ShaderStage::TessControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case ShaderStage::TessEval:    return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    }
    return VK_SHADER_STAGE_VERTEX_BIT;
}

VkPipelineShaderStageCreateInfo Shader::stageInfo() const
{
    VkPipelineShaderStageCreateInfo si{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    si.stage  = vkStage();
    si.module = m_module;
    si.pName  = "main";
    return si;
}

} // namespace fusion
