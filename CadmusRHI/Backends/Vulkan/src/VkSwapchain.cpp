#include "VkBackend.hpp"
#include "VkSwapchain.hpp"
#include "Helpers/VkHelper.hpp"

namespace rhi::vulkan
{
    VkSwapchain::VkSwapchain(VkDevice InDevice, VkPhysicalDevice InPhysicalDevice, VkSurfaceKHR InSurface)
    {
        Device = InDevice;
        FSwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(InPhysicalDevice, InSurface);
        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapchainSupport.Formats);
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
        VkExtent2D extent = swapchainSupport.Capabilities.currentExtent;

        Desc.BufferCount = swapchainSupport.Capabilities.minImageCount + 1;
        if (swapchainSupport.Capabilities.maxImageCount > 0 && Desc.BufferCount > swapchainSupport.Capabilities.maxImageCount)
        {
            Desc.BufferCount = swapchainSupport.Capabilities.maxImageCount;
        }

        Desc.Extent.Width = extent.width;
        Desc.Extent.Height = extent.height;


        VkSwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = InSurface;
        swapchainCreateInfo.minImageCount = Desc.BufferCount;
        swapchainCreateInfo.imageFormat = surfaceFormat.format;
        swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapchainCreateInfo.imageExtent = extent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.preTransform = swapchainSupport.Capabilities.currentTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = presentMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
        Desc.VSync = presentMode == VK_PRESENT_MODE_FIFO_KHR;
        Desc.Format = VkFormatToRHIFormat(surfaceFormat.format);

        VK_CHECK_RESULT(vkCreateSwapchainKHR(InDevice, &swapchainCreateInfo, nullptr, &Swapchain));

        uint32_t imageCount = Desc.BufferCount;
        VK_CHECK_RESULT(vkGetSwapchainImagesKHR(InDevice, Swapchain, &imageCount, nullptr));
        if (imageCount == 0)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "vkGetSwapchainImagesKHR returned zero swapchain images.");
            Desc.BufferCount = 0;
            return;
        }

        Desc.BufferCount = imageCount;
        SwapChainImages.resize(Desc.BufferCount);
        VK_CHECK_RESULT(vkGetSwapchainImagesKHR(InDevice, Swapchain, &imageCount, SwapChainImages.data()));
        Desc.BufferCount = imageCount;
        
        SwapChainImageViews.resize(Desc.BufferCount);
        for (size_t i = 0; i < Desc.BufferCount; i++)
        {
            if (SwapChainImages[i] == VK_NULL_HANDLE)
            {
                LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Swapchain image handle is null.");
                continue;
            }

            VkImageViewCreateInfo viewCreateInfo{};
            viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewCreateInfo.image = SwapChainImages[i];
            viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewCreateInfo.format = surfaceFormat.format;
            viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewCreateInfo.subresourceRange.baseMipLevel = 0;
            viewCreateInfo.subresourceRange.levelCount = 1;
            viewCreateInfo.subresourceRange.baseArrayLayer = 0;
            viewCreateInfo.subresourceRange.layerCount = 1;

            VK_CHECK_RESULT(vkCreateImageView(InDevice, &viewCreateInfo, nullptr, &SwapChainImageViews[i]));
        }

        const FTextureCreateInfo swapchainTextureCreateInfo = FTextureCreateInfo::Texture2D(
            Desc.Format,
            Desc.Extent,
            1,
            1,
            1,
            ETextureUsageFlags::ColorAttachment | ETextureUsageFlags::Sampled);

        SwapchainTextures.reserve(Desc.BufferCount);
        for (size_t i = 0; i < Desc.BufferCount; ++i)
        {
            if (SwapChainImages[i] == VK_NULL_HANDLE || SwapChainImageViews[i] == VK_NULL_HANDLE)
            {
                LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Skipping invalid swapchain image while creating texture wrappers.");
                continue;
            }

            SwapchainTextures.emplace_back(
                Device,
                SwapChainImages[i],
                SwapChainImageViews[i],
                swapchainTextureCreateInfo,
                false,
                false);
        }

        Desc.BufferCount = static_cast<uint32_t>(SwapchainTextures.size());
    }

    VkSwapchain::~VkSwapchain()
    {
        SwapchainTextures.clear();

        for (VkImageView imageView : SwapChainImageViews)
        {
            if (imageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(Device, imageView, nullptr);
            }
        }
        SwapChainImageViews.clear();

        if (Swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(Device, Swapchain, nullptr);
            Swapchain = VK_NULL_HANDLE;
        }

        SwapChainImages.clear();
    }

    const FSwapchainDesc &VkSwapchain::GetDesc() const
    {
        return Desc;
    }

    VkVulkanTexture* VkSwapchain::GetCurrentSwapchainTexture()
    {
        return GetSwapchainTexture(CurrentImageIndex);
    }

    VkVulkanTexture* VkSwapchain::GetSwapchainTexture(uint32_t ImageIndex)
    {
        if (ImageIndex >= SwapchainTextures.size())
        {
            return nullptr;
        }

        return &SwapchainTextures[ImageIndex];
    }

    uint32_t VkSwapchain::GetCurrentImageIndex() const
    {
        return CurrentImageIndex;
    }

    void VkSwapchain::SetCurrentImageIndex(uint32_t ImageIndex)
    {
        if (SwapchainTextures.empty())
        {
            CurrentImageIndex = 0;
            return;
        }

        CurrentImageIndex = ImageIndex % static_cast<uint32_t>(SwapchainTextures.size());
    }
}
