#pragma once

#include <Swapchain.hpp>
#include <vulkan/vulkan_core.h>
#include <vector>

#include "Resources/VkTexture.hpp"

namespace rhi::vulkan
{
    class VkSwapchain : public ISwapchain
    {
    public:
        explicit VkSwapchain(VkDevice InDevice, VkPhysicalDevice InPhysicalDevice, VkSurfaceKHR InSurface);
        ~VkSwapchain() override;

        const FSwapchainDesc &GetDesc() const override;
        VkVulkanTexture* GetCurrentSwapchainTexture();
        VkVulkanTexture* GetSwapchainTexture(uint32_t ImageIndex);
        uint32_t GetCurrentImageIndex() const;
        void SetCurrentImageIndex(uint32_t ImageIndex);
        VkSwapchainKHR GetHandle() const { return Swapchain; }

    private:
        FSwapchainDesc Desc;

        VkSwapchainKHR Swapchain;
        VkDevice Device;
        std::vector<VkImage> SwapChainImages;
        std::vector<VkImageView> SwapChainImageViews;
        std::vector<VkVulkanTexture> SwapchainTextures;
        uint32_t CurrentImageIndex{0};

    };
}