#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace fusion {

class Device;

enum class ShaderStage {
    Vertex,
    Fragment,
    Compute,
    Geometry,
    TessControl,
    TessEval,
};

class Shader {
public:
    Shader(const Device& device, const std::string& spirvPath, ShaderStage stage);
    Shader(const Device& device, const std::vector<uint32_t>& spirv, ShaderStage stage);
    ~Shader();

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    VkShaderModule             module() const { return m_module; }
    ShaderStage                stage()  const { return m_stage; }
    VkShaderStageFlagBits      vkStage() const;
    VkPipelineShaderStageCreateInfo stageInfo() const;

private:
    void create(const Device& device, const std::vector<uint32_t>& spirv);

    VkDevice       m_device = VK_NULL_HANDLE;
    VkShaderModule m_module = VK_NULL_HANDLE;
    ShaderStage    m_stage{};
};

} // namespace fusion
