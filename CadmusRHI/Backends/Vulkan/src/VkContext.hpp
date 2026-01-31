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

        const std::vector<std::unique_ptr<IGxAdapter>>& GetAdapters() const { return Adapters; }

    private:
        void ReleaseInstance();
        bool CreateInstance();
        void EnumerateAdapters();

        VkInstance Instance{VK_NULL_HANDLE};
        std::vector<std::unique_ptr<IGxAdapter>> Adapters;
    };
}
