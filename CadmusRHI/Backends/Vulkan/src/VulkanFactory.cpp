#include "VkBackend.hpp"
#include "VkContext.hpp"

extern "C" CADMUS_VULKAN_RHI_API rhi::IContext* CreateVulkanContext()
{
    return new rhi::vulkan::FVulkanContext();
}
