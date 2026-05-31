#include "fusion/resources/Sampler.hpp"
#include "fusion/core/Device.hpp"
#include <stdexcept>

namespace fusion {

Sampler::Sampler(const Device& device, const SamplerConfig& cfg)
    : m_device(device.handle())
{
    VkSamplerCreateInfo ci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    ci.magFilter               = cfg.magFilter;
    ci.minFilter               = cfg.minFilter;
    ci.mipmapMode              = cfg.mipMode;
    ci.addressModeU            = cfg.addressMode;
    ci.addressModeV            = cfg.addressMode;
    ci.addressModeW            = cfg.addressMode;
    ci.anisotropyEnable        = cfg.maxAnisotropy > 1.0f ? VK_TRUE : VK_FALSE;
    ci.maxAnisotropy           = cfg.maxAnisotropy;
    ci.compareEnable           = VK_FALSE;
    ci.compareOp               = VK_COMPARE_OP_ALWAYS;
    ci.minLod                  = 0.0f;
    ci.maxLod                  = cfg.maxLod;
    ci.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    ci.unnormalizedCoordinates = cfg.unnormalized ? VK_TRUE : VK_FALSE;

    if (vkCreateSampler(m_device, &ci, nullptr, &m_sampler) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create sampler");
}

Sampler::~Sampler()
{
    vkDestroySampler(m_device, m_sampler, nullptr);
}

} // namespace fusion
