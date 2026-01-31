#include "VkBackend.hpp"

namespace rhi::vulkan
{
    FVulkanSwapchain::FVulkanSwapchain(FSwapchainDesc desc)
        : Desc(desc)
    {
    }

    const FSwapchainDesc& FVulkanSwapchain::GetDesc() const
    {
        return Desc;
    }
}
