#pragma once

#include "Defs.hpp"

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "Context.hpp"
#include "GxAdapter.hpp"
#include "Swapchain.hpp"
#include "Types.hpp"



namespace rhi::vulkan
{
    extern rhi::PFN_LoggerFunc gLoggerFunc;

    class FVulkanGxAdapter : public IGxAdapter
    {
    public:
        explicit FVulkanGxAdapter(FGxAdapterInfo info);
        const FGxAdapterInfo& GetInfo() const override;

    private:
        FGxAdapterInfo Info;
    };

    class FVulkanSwapchain : public ISwapchain
    {
    public:
        explicit FVulkanSwapchain(FSwapchainDesc desc);
        const FSwapchainDesc& GetDesc() const override;

    private:
        FSwapchainDesc Desc;
    };
}

extern "C" CADMUS_VULKAN_RHI_API rhi::IContext* CreateRHIContext();
extern "C" CADMUS_VULKAN_RHI_API void SetRHILoggerFunc(rhi::PFN_LoggerFunc loggerFunc);
