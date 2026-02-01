#pragma once

#include "VkBackend.hpp"
#include "VkSwapchain.hpp"
#include "Helpers/Types.hpp"
#include "VkAdapter.hpp"

#define VK_MAX_QUEUE_FAMILIES_PER_TYPE 8

namespace rhi::vulkan
{

    class FVulkanContext : public IContext
    {
    public:
        FVulkanContext();
        ~FVulkanContext() override;

        bool Initialize(const FContextInitParams &Params) override;

    private:
        uint32_t SelectPhysicalDevice(const std::vector<VkPhysicalDevice> &physicalDevices, int desiredIdx = -1);
        void ReleaseInstance();
        bool CreateInstance();
        void EnumerateAdapters();

        VkInstance Instance{VK_NULL_HANDLE};
        VkPhysicalDevice PhysicalDevice{VK_NULL_HANDLE};

        VkSurfaceKHR Surface{VK_NULL_HANDLE};
        std::vector<FGxAdapterInfo> AdapterInfos;

        VkGraphicsAdapter *SelectedAdapter{nullptr};
        VkSwapchain *Swapchain{nullptr};
    };
}
