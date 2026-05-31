#pragma once
#include <vulkan/vulkan.h>

namespace fusion {

class Device;

struct SamplerConfig {
    VkFilter             magFilter  = VK_FILTER_LINEAR;
    VkFilter             minFilter  = VK_FILTER_LINEAR;
    VkSamplerMipmapMode  mipMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    float                maxAnisotropy = 16.0f;
    float                maxLod        = VK_LOD_CLAMP_NONE;
    bool                 unnormalized  = false;
};

class Sampler {
public:
    explicit Sampler(const Device& device, const SamplerConfig& cfg = {});
    ~Sampler();

    Sampler(const Sampler&)            = delete;
    Sampler& operator=(const Sampler&) = delete;

    VkSampler handle() const { return m_sampler; }

private:
    VkDevice  m_device  = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
};

} // namespace fusion
