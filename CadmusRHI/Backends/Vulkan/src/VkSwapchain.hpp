#pragma once

#include <Swapchain.hpp>
#include <vulkan/vulkan_core.h>
#include <vector>

namespace rhi::vulkan
{
    class VkSwapchain : public ISwapchain
    {
    public:
        explicit VkSwapchain(VkDevice InDevice, VkPhysicalDevice InPhysicalDevice, VkSurfaceKHR InSurface);
        ~VkSwapchain() override;

        const FSwapchainDesc &GetDesc() const override;

    private:
        FSwapchainDesc Desc;

        VkSwapchainKHR Swapchain;
        VkDevice Device;
        std::vector<VkImage> SwapChainImages;
        std::vector<VkImageView> SwapChainImageViews;

    };
}