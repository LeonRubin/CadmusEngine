#include "VkBackend.hpp"

#include <utility>

namespace rhi::vulkan
{
    FVulkanGxAdapter::FVulkanGxAdapter(FGxAdapterInfo info)
        : Info(std::move(info))
    {
    }

    const FGxAdapterInfo& FVulkanGxAdapter::GetInfo() const
    {
        return Info;
    }
}
