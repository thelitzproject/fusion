#include "fusion/render/PipelineCache.hpp"
#include "fusion/core/Device.hpp"
#include <fstream>
#include <stdexcept>

namespace fusion {

PipelineCache::PipelineCache(const Device& device, const std::filesystem::path& cacheFile)
    : m_device(device.handle())
{
    std::vector<char> initialData;
    if (!cacheFile.empty()) {
        std::ifstream f(cacheFile, std::ios::binary | std::ios::ate);
        if (f.is_open()) {
            initialData.resize(static_cast<size_t>(f.tellg()));
            f.seekg(0);
            f.read(initialData.data(), static_cast<std::streamsize>(initialData.size()));
        }
    }

    VkPipelineCacheCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    ci.initialDataSize = initialData.size();
    ci.pInitialData    = initialData.empty() ? nullptr : initialData.data();

    if (vkCreatePipelineCache(m_device, &ci, nullptr, &m_cache) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create pipeline cache");
}

PipelineCache::~PipelineCache()
{
    vkDestroyPipelineCache(m_device, m_cache, nullptr);
}

void PipelineCache::save(const std::filesystem::path& path) const
{
    size_t dataSize = 0;
    vkGetPipelineCacheData(m_device, m_cache, &dataSize, nullptr);

    std::vector<char> data(dataSize);
    vkGetPipelineCacheData(m_device, m_cache, &dataSize, data.data());

    std::ofstream f(path, std::ios::binary);
    if (!f.is_open())
        throw std::runtime_error("fusion: failed to write pipeline cache: " + path.string());
    f.write(data.data(), static_cast<std::streamsize>(data.size()));
}

} // namespace fusion
