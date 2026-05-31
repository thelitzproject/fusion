#pragma once
#include <vulkan/vulkan.h>
#include <filesystem>
#include <vector>

namespace fusion {

class Device;

class PipelineCache {
public:
    explicit PipelineCache(const Device& device,
                           const std::filesystem::path& cacheFile = {});
    ~PipelineCache();

    PipelineCache(const PipelineCache&)            = delete;
    PipelineCache& operator=(const PipelineCache&) = delete;

    VkPipelineCache handle() const { return m_cache; }

    void save(const std::filesystem::path& path) const;

private:
    VkDevice        m_device = VK_NULL_HANDLE;
    VkPipelineCache m_cache  = VK_NULL_HANDLE;
};

} // namespace fusion
