#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include <stdexcept>

// ── Headless Vulkan helpers ───────────────────────────────────────────────────
// These tests exercise the GPU path without creating a window or surface.
// They link directly against Vulkan + VMA, not the fusion library, so GLFW is
// never loaded and the tests run safely in headless CI environments.

static VkInstance createInstance()
{
    VkApplicationInfo ai{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    ai.pApplicationName = "fusion-integration-test";
    ai.apiVersion       = VK_API_VERSION_1_2;

    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &ai;

    VkInstance inst = VK_NULL_HANDLE;
    if (vkCreateInstance(&ci, nullptr, &inst) != VK_SUCCESS)
        throw std::runtime_error("vkCreateInstance failed");
    return inst;
}

static VkPhysicalDevice pickPhysicalDevice(VkInstance inst)
{
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(inst, &count, nullptr);
    if (count == 0) throw std::runtime_error("no Vulkan GPU");

    std::vector<VkPhysicalDevice> devs(count);
    vkEnumeratePhysicalDevices(inst, &count, devs.data());
    return devs[0]; // just pick the first
}

static VkDevice createDevice(VkPhysicalDevice phys, uint32_t& outFamily)
{
    uint32_t qCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phys, &qCount, nullptr);
    std::vector<VkQueueFamilyProperties> qprops(qCount);
    vkGetPhysicalDeviceQueueFamilyProperties(phys, &qCount, qprops.data());

    outFamily = UINT32_MAX;
    for (uint32_t i = 0; i < qCount; ++i) {
        if (qprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            outFamily = i;
            break;
        }
    }
    if (outFamily == UINT32_MAX) throw std::runtime_error("no graphics queue");

    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = outFamily;
    qci.queueCount       = 1;
    qci.pQueuePriorities = &priority;

    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos    = &qci;

    VkDevice dev = VK_NULL_HANDLE;
    if (vkCreateDevice(phys, &dci, nullptr, &dev) != VK_SUCCESS)
        throw std::runtime_error("vkCreateDevice failed");
    return dev;
}

// ── Test: physical device selection ──────────────────────────────────────────

static void testPhysicalDevicePick()
{
    VkInstance inst = createInstance();
    VkPhysicalDevice phys = pickPhysicalDevice(inst);
    assert(phys != VK_NULL_HANDLE);

    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(phys, &props);
    std::cout << "[PASS] physical device: " << props.deviceName << "\n";

    vkDestroyInstance(inst, nullptr);
}

// ── Test: logical device + VMA allocator ─────────────────────────────────────

static void testDeviceAndAllocator()
{
    VkInstance     inst    = createInstance();
    VkPhysicalDevice phys  = pickPhysicalDevice(inst);
    uint32_t       family  = 0;
    VkDevice       dev     = createDevice(phys, family);

    VmaAllocatorCreateInfo aci{};
    aci.vulkanApiVersion = VK_API_VERSION_1_2;
    aci.instance         = inst;
    aci.physicalDevice   = phys;
    aci.device           = dev;

    VmaAllocator vma = VK_NULL_HANDLE;
    if (vmaCreateAllocator(&aci, &vma) != VK_SUCCESS)
        throw std::runtime_error("vmaCreateAllocator failed");
    assert(vma != VK_NULL_HANDLE);

    vmaDestroyAllocator(vma);
    vkDestroyDevice(dev, nullptr);
    vkDestroyInstance(inst, nullptr);
    std::cout << "[PASS] device and VMA allocator creation\n";
}

// ── Test: GPU buffer allocation and CPU upload ────────────────────────────────

static void testBufferAlloc()
{
    VkInstance     inst   = createInstance();
    VkPhysicalDevice phys = pickPhysicalDevice(inst);
    uint32_t       family = 0;
    VkDevice       dev    = createDevice(phys, family);

    VmaAllocatorCreateInfo aci{};
    aci.vulkanApiVersion = VK_API_VERSION_1_2;
    aci.instance         = inst;
    aci.physicalDevice   = phys;
    aci.device           = dev;
    VmaAllocator vma = VK_NULL_HANDLE;
    vmaCreateAllocator(&aci, &vma);

    static const uint32_t src[4] = { 10, 20, 30, 40 };

    VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bci.size        = sizeof(src);
    bci.usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCI{};
    allocCI.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer      buf   = VK_NULL_HANDLE;
    VmaAllocation alloc = VK_NULL_HANDLE;
    VmaAllocationInfo info{};
    if (vmaCreateBuffer(vma, &bci, &allocCI, &buf, &alloc, &info) != VK_SUCCESS)
        throw std::runtime_error("vmaCreateBuffer failed");

    assert(buf != VK_NULL_HANDLE);
    assert(info.pMappedData != nullptr);

    std::memcpy(info.pMappedData, src, sizeof(src));
    vmaFlushAllocation(vma, alloc, 0, sizeof(src));

    // Verify round-trip
    assert(std::memcmp(info.pMappedData, src, sizeof(src)) == 0);

    vmaDestroyBuffer(vma, buf, alloc);
    vmaDestroyAllocator(vma);
    vkDestroyDevice(dev, nullptr);
    vkDestroyInstance(inst, nullptr);
    std::cout << "[PASS] buffer allocation and upload\n";
}

// ── Test: fence creation ──────────────────────────────────────────────────────

static void testFence()
{
    VkInstance     inst   = createInstance();
    VkPhysicalDevice phys = pickPhysicalDevice(inst);
    uint32_t       family = 0;
    VkDevice       dev    = createDevice(phys, family);

    VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkFence fence = VK_NULL_HANDLE;
    vkCreateFence(dev, &fci, nullptr, &fence);
    assert(vkGetFenceStatus(dev, fence) == VK_SUCCESS);

    vkDestroyFence(dev, fence, nullptr);
    vkDestroyDevice(dev, nullptr);
    vkDestroyInstance(inst, nullptr);
    std::cout << "[PASS] fence create and status\n";
}

int main()
{
    uint32_t count = 0;
    VkInstance probe = VK_NULL_HANDLE;
    {
        VkApplicationInfo ai{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        ai.apiVersion = VK_API_VERSION_1_2;
        VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        ci.pApplicationInfo = &ai;
        if (vkCreateInstance(&ci, nullptr, &probe) != VK_SUCCESS) {
            std::cout << "[SKIP] Vulkan instance creation failed\n";
            return 0;
        }
        vkEnumeratePhysicalDevices(probe, &count, nullptr);
        vkDestroyInstance(probe, nullptr);
    }

    if (count == 0) {
        std::cout << "[SKIP] no Vulkan-capable GPU found\n";
        return 0;
    }

    try {
        testPhysicalDevicePick();
        testDeviceAndAllocator();
        testBufferAlloc();
        testFence();
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] " << e.what() << '\n';
        return 1;
    }

    std::cout << "integration: all tests passed\n";
    return 0;
}
