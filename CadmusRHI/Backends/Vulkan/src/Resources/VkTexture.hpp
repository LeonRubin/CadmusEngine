#pragma once

#include "Resources/Texture.hpp"
#include <vulkan/vulkan_core.h>

namespace rhi::vulkan
{
    class VkVulkanTexture final : public rhi::ITexture
    {
    public:
        VkVulkanTexture(VkDevice InDevice, VkPhysicalDevice InPhysicalDevice, const FTextureCreateInfo& InCreateInfo);
        VkVulkanTexture(VkDevice InDevice, VkImage InImage, VkImageView InImageView, const FTextureCreateInfo& InCreateInfo, bool bInOwnsImage = false, bool bInOwnsImageView = true);
        VkVulkanTexture(const VkVulkanTexture&) = delete;
        VkVulkanTexture& operator=(const VkVulkanTexture&) = delete;
        VkVulkanTexture(VkVulkanTexture&& Other) noexcept;
        VkVulkanTexture& operator=(VkVulkanTexture&& Other) noexcept;

        ~VkVulkanTexture() override;

        // Getters for Vulkan resources
        VkImage GetImage() const { return Image; }
        VkImageView GetImageView() const { return ImageView; }
        VkFormat GetVkFormat() const { return Format; }
        VkDevice GetDevice() const { return Device; }
        VkDeviceMemory GetDeviceMemory() const { return DeviceMemory; }
        const VkMemoryRequirements& GetMemoryRequirements() const { return MemoryRequirements; }
        VkImageSubresourceRange GetSubresourceRange() const { return SubresourceRange; }
        VkImageLayout GetCurrentLayout() const { return CurrentLayout; }
        void SetCurrentLayout(VkImageLayout InLayout) { CurrentLayout = InLayout; }
        bool IsSwapchainImage() const { return !bOwnsImage && !bOwnsDeviceMemory; }

    private:
        static uint32_t FindMemoryType(VkPhysicalDevice InPhysicalDevice, uint32_t TypeFilter, VkMemoryPropertyFlags Properties);
        void Release();

        friend class VkVulkanTextureFactory; // Allow factory to set private members

        // Vulkan resources
        VkImage Image{VK_NULL_HANDLE};
        VkImageView ImageView{VK_NULL_HANDLE};
        VkDeviceMemory DeviceMemory{VK_NULL_HANDLE};
        VkFormat Format{VK_FORMAT_UNDEFINED};
        VkDevice Device{VK_NULL_HANDLE};
        
        // Memory tracking and additional metadata
        VkMemoryRequirements MemoryRequirements{0, 0, 0};
        VkImageSubresourceRange SubresourceRange{};
        VkImageLayout CurrentLayout{VK_IMAGE_LAYOUT_UNDEFINED};
        VkImageAspectFlags AspectMask{VK_IMAGE_ASPECT_COLOR_BIT};
        bool bOwnsImage{true};
        bool bOwnsImageView{true};
        bool bOwnsDeviceMemory{true};
    };
}