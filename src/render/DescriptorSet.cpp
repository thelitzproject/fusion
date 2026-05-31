#include "fusion/render/DescriptorSet.hpp"
#include "fusion/resources/Buffer.hpp"
#include "fusion/resources/Image.hpp"
#include "fusion/resources/Sampler.hpp"
#include "fusion/core/Device.hpp"
#include <stdexcept>

namespace fusion {

DescriptorSetLayout::DescriptorSetLayout(const Device& device,
    const std::vector<DescriptorBinding>& bindings)
    : m_device(device.handle())
{
    std::vector<VkDescriptorSetLayoutBinding> vkBindings;
    for (const auto& b : bindings) {
        VkDescriptorSetLayoutBinding lb{};
        lb.binding            = b.binding;
        lb.descriptorType     = b.type;
        lb.descriptorCount    = b.count;
        lb.stageFlags         = b.stages;
        vkBindings.push_back(lb);
    }

    VkDescriptorSetLayoutCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    ci.bindingCount = static_cast<uint32_t>(vkBindings.size());
    ci.pBindings    = vkBindings.data();

    if (vkCreateDescriptorSetLayout(m_device, &ci, nullptr, &m_layout) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to create descriptor set layout");
}

DescriptorSetLayout::~DescriptorSetLayout()
{
    vkDestroyDescriptorSetLayout(m_device, m_layout, nullptr);
}

DescriptorSet::DescriptorSet(const Device& device,
    VkDescriptorPool pool, VkDescriptorSetLayout layout)
    : m_device(device.handle())
{
    VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    ai.descriptorPool     = pool;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts        = &layout;

    if (vkAllocateDescriptorSets(m_device, &ai, &m_set) != VK_SUCCESS)
        throw std::runtime_error("fusion: failed to allocate descriptor set");
}

void DescriptorSet::update(const std::vector<DescriptorWrite>& writes)
{
    std::vector<VkWriteDescriptorSet> vkWrites;
    std::vector<VkDescriptorBufferInfo> bufInfos;
    std::vector<VkDescriptorImageInfo>  imgInfos;
    bufInfos.reserve(writes.size());
    imgInfos.reserve(writes.size());

    for (const auto& w : writes) {
        VkWriteDescriptorSet vw{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        vw.dstSet         = m_set;
        vw.dstBinding     = w.binding;
        vw.descriptorType = w.type;
        vw.descriptorCount = 1;

        if (std::holds_alternative<BufferWrite>(w.data)) {
            const auto& bw = std::get<BufferWrite>(w.data);
            bufInfos.push_back({ bw.buffer->handle(), bw.offset, bw.range });
            vw.pBufferInfo = &bufInfos.back();
        } else {
            const auto& iw = std::get<ImageWrite>(w.data);
            imgInfos.push_back({
                iw.sampler ? iw.sampler->handle() : VK_NULL_HANDLE,
                iw.image->view(),
                iw.layout
            });
            vw.pImageInfo = &imgInfos.back();
        }
        vkWrites.push_back(vw);
    }

    vkUpdateDescriptorSets(m_device,
        static_cast<uint32_t>(vkWrites.size()), vkWrites.data(), 0, nullptr);
}

} // namespace fusion
