#include "fusion/core/Device.hpp"
#include <stdexcept>

namespace fusion {

void Queue::submit(const VkSubmitInfo& info, VkFence fence) const
{
    if (vkQueueSubmit(m_queue, 1, &info, fence) != VK_SUCCESS)
        throw std::runtime_error("fusion: queue submit failed");
}

void Queue::waitIdle() const
{
    vkQueueWaitIdle(m_queue);
}

} // namespace fusion
