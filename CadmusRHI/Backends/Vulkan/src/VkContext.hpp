#pragma once

#include "VkBackend.hpp"

namespace rhi::vulkan
{
    class FVulkanContext : public IContext
    {
    public:
        FVulkanContext();
        ~FVulkanContext() override;

        bool Initialize(const FContextInitParams& Params) override;

    private:

        void ReleaseInstance();
        bool CreateInstance();
        void EnumerateAdapters();

        VkInstance Instance{VK_NULL_HANDLE};
        VkPhysicalDevice PhysicalDevice{VK_NULL_HANDLE};
        std::vector<FGxAdapterInfo> AdapterInfos;
    };
}
