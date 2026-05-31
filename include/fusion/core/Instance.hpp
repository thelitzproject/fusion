#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace fusion {

struct InstanceConfig {
    std::string appName    = "fusion-app";
    uint32_t    appVersion = VK_MAKE_VERSION(1, 0, 0);
    bool        validation = false;
    std::vector<const char*> extraExtensions;
};

class Instance {
public:
    explicit Instance(const InstanceConfig& cfg);
    ~Instance();

    Instance(const Instance&)            = delete;
    Instance& operator=(const Instance&) = delete;
    Instance(Instance&&)                 = delete;
    Instance& operator=(Instance&&)      = delete;

    VkInstance handle() const { return m_instance; }
    bool       validationEnabled() const { return m_validation; }

private:
    void createInstance(const InstanceConfig& cfg);
    void setupDebugMessenger();

    VkInstance               m_instance       = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    bool                     m_validation     = false;
};

} // namespace fusion
