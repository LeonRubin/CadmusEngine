#include "VkTexture.hpp"

#include <algorithm>

#include "Helpers/VkHelper.hpp"

namespace rhi::vulkan
{
    uint32_t VkVulkanTexture::FindMemoryType(VkPhysicalDevice InPhysicalDevice, uint32_t TypeFilter, VkMemoryPropertyFlags Properties)
    {
        VkPhysicalDeviceMemoryProperties MemoryProperties{};
        vkGetPhysicalDeviceMemoryProperties(InPhysicalDevice, &MemoryProperties);

        for (uint32_t index = 0; index < MemoryProperties.memoryTypeCount; ++index)
        {
            const bool TypeMatches = (TypeFilter & (1u << index)) != 0;
            const bool PropertyMatches = (MemoryProperties.memoryTypes[index].propertyFlags & Properties) == Properties;
            if (TypeMatches && PropertyMatches)
            {
                return index;
            }
        }

        LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Failed to find suitable Vulkan memory type for texture.");
        assert(false && "No suitable Vulkan memory type found for texture");
        return 0;
    }

    void VkVulkanTexture::Release()
    {
        if (Device == VK_NULL_HANDLE)
        {
            return;
        }

        if (bOwnsImageView && ImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(Device, ImageView, nullptr);
            ImageView = VK_NULL_HANDLE;
        }

        if (bOwnsImage && Image != VK_NULL_HANDLE)
        {
            vkDestroyImage(Device, Image, nullptr);
            Image = VK_NULL_HANDLE;
        }

        if (bOwnsDeviceMemory && DeviceMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(Device, DeviceMemory, nullptr);
            DeviceMemory = VK_NULL_HANDLE;
        }

        Device = VK_NULL_HANDLE;
        Format = VK_FORMAT_UNDEFINED;
        MemoryRequirements = {0, 0, 0};
        SubresourceRange = {};
        CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        AspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bOwnsImage = true;
        bOwnsImageView = true;
        bOwnsDeviceMemory = true;
    }

    VkVulkanTexture::VkVulkanTexture(VkDevice InDevice, VkPhysicalDevice InPhysicalDevice, const FTextureCreateInfo &InCreateInfo)
    {
        Device = InDevice;
        CreateInfo = InCreateInfo;
        Format = ConvertColorFormat(InCreateInfo.Format);
        const uint32_t mipLevels = std::max(1u, InCreateInfo.MipLevels);
        const uint32_t arrayLayers = std::max(1u, InCreateInfo.ArrayLayers);
        const uint32_t depth = std::max(1u, InCreateInfo.Extent.Depth);
        AspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        SubresourceRange.aspectMask = AspectMask;
        SubresourceRange.baseMipLevel = 0;
        SubresourceRange.levelCount = mipLevels;
        SubresourceRange.baseArrayLayer = 0;
        SubresourceRange.layerCount = InCreateInfo.Type == ETextureType::Texture3D ? 1u : arrayLayers;
        CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = ConvertImageType(InCreateInfo.Type);
        imageCreateInfo.format = Format;
        imageCreateInfo.extent.width = std::max(1u, InCreateInfo.Extent.Width);
        imageCreateInfo.extent.height = std::max(1u, InCreateInfo.Extent.Height);
        imageCreateInfo.extent.depth = depth;
        imageCreateInfo.mipLevels = mipLevels;
        imageCreateInfo.arrayLayers = InCreateInfo.Type == ETextureType::Texture3D ? 1u : arrayLayers;
        imageCreateInfo.samples = ConvertSampleCount(InCreateInfo.SampleCount);
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = ConvertTextureUsageFlags(InCreateInfo.Usage);
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VK_CHECK_RESULT(vkCreateImage(Device, &imageCreateInfo, nullptr, &Image));

        vkGetImageMemoryRequirements(Device, Image, &MemoryRequirements);

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = MemoryRequirements.size;
        allocateInfo.memoryTypeIndex = FindMemoryType(InPhysicalDevice, MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK_RESULT(vkAllocateMemory(Device, &allocateInfo, nullptr, &DeviceMemory));
        VK_CHECK_RESULT(vkBindImageMemory(Device, Image, DeviceMemory, 0));

        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = Image;
        imageViewCreateInfo.viewType = ConvertImageViewType(InCreateInfo.Type);
        imageViewCreateInfo.format = Format;
        imageViewCreateInfo.subresourceRange = SubresourceRange;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        VK_CHECK_RESULT(vkCreateImageView(Device, &imageViewCreateInfo, nullptr, &ImageView));
    }

    VkVulkanTexture::VkVulkanTexture(VkDevice InDevice, VkImage InImage, VkImageView InImageView, const FTextureCreateInfo& InCreateInfo, bool bInOwnsImage, bool bInOwnsImageView)
    {
        Device = InDevice;
        CreateInfo = InCreateInfo;
        Image = InImage;
        ImageView = InImageView;
        DeviceMemory = VK_NULL_HANDLE;
        Format = ConvertColorFormat(InCreateInfo.Format);
        AspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        SubresourceRange.aspectMask = AspectMask;
        SubresourceRange.baseMipLevel = 0;
        SubresourceRange.levelCount = std::max(1u, InCreateInfo.MipLevels);
        SubresourceRange.baseArrayLayer = 0;
        SubresourceRange.layerCount = std::max(1u, InCreateInfo.ArrayLayers);
        CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        MemoryRequirements = {0, 0, 0};
        bOwnsImage = bInOwnsImage;
        bOwnsImageView = bInOwnsImageView;
        bOwnsDeviceMemory = false;
    }

    VkVulkanTexture::VkVulkanTexture(VkVulkanTexture&& Other) noexcept
        : Image(Other.Image)
        , ImageView(Other.ImageView)
        , DeviceMemory(Other.DeviceMemory)
        , Format(Other.Format)
        , Device(Other.Device)
        , MemoryRequirements(Other.MemoryRequirements)
        , SubresourceRange(Other.SubresourceRange)
        , CurrentLayout(Other.CurrentLayout)
        , AspectMask(Other.AspectMask)
        , bOwnsImage(Other.bOwnsImage)
        , bOwnsImageView(Other.bOwnsImageView)
        , bOwnsDeviceMemory(Other.bOwnsDeviceMemory)
    {
        CreateInfo = Other.CreateInfo;
        Other.Image = VK_NULL_HANDLE;
        Other.ImageView = VK_NULL_HANDLE;
        Other.DeviceMemory = VK_NULL_HANDLE;
        Other.Format = VK_FORMAT_UNDEFINED;
        Other.Device = VK_NULL_HANDLE;
        Other.MemoryRequirements = {0, 0, 0};
        Other.SubresourceRange = {};
        Other.CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        Other.AspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        Other.bOwnsImage = false;
        Other.bOwnsImageView = false;
        Other.bOwnsDeviceMemory = false;
    }

    VkVulkanTexture& VkVulkanTexture::operator=(VkVulkanTexture&& Other) noexcept
    {
        if (this != &Other)
        {
            Release();

            CreateInfo = Other.CreateInfo;
            Image = Other.Image;
            ImageView = Other.ImageView;
            DeviceMemory = Other.DeviceMemory;
            Format = Other.Format;
            Device = Other.Device;
            MemoryRequirements = Other.MemoryRequirements;
            SubresourceRange = Other.SubresourceRange;
            CurrentLayout = Other.CurrentLayout;
            AspectMask = Other.AspectMask;
            bOwnsImage = Other.bOwnsImage;
            bOwnsImageView = Other.bOwnsImageView;
            bOwnsDeviceMemory = Other.bOwnsDeviceMemory;

            Other.Image = VK_NULL_HANDLE;
            Other.ImageView = VK_NULL_HANDLE;
            Other.DeviceMemory = VK_NULL_HANDLE;
            Other.Format = VK_FORMAT_UNDEFINED;
            Other.Device = VK_NULL_HANDLE;
            Other.MemoryRequirements = {0, 0, 0};
            Other.SubresourceRange = {};
            Other.CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            Other.AspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            Other.bOwnsImage = false;
            Other.bOwnsImageView = false;
            Other.bOwnsDeviceMemory = false;
        }

        return *this;
    }

    VkVulkanTexture::~VkVulkanTexture()
    {
        Release();
    }

}