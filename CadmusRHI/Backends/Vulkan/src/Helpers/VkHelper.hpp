#pragma once

#include <cassert>
#include <cstdio>
#include <algorithm>

#include <Context.hpp>
#include <vulkan/vulkan_core.h>
#include <Types.hpp>
#include <vector>

#include "Types.hpp"

#define VK_CHECK_RESULT(fnc)                                                               \
    do                                                                                     \
    {                                                                                      \
        VkResult res = (fnc);                                                              \
        if (res != VK_SUCCESS)                                                             \
        {                                                                                  \
            if (gLoggerFunc)                                                               \
            {                                                                              \
                char buf[256];                                                             \
                std::snprintf(buf, sizeof(buf), "Vulkan error at %s:%d in %s returned %d", \
                              __FILE__, __LINE__, #fnc, static_cast<int>(res));            \
                gLoggerFunc(rhi::RHI_LOGLEVEL_ERROR, buf);                                 \
            }                                                                              \
            assert(res == VK_SUCCESS);                                                     \
        }                                                                                  \
    } while (0)
namespace rhi::vulkan
{
    extern rhi::PFN_LoggerFunc gLoggerFunc;

    inline void LogRHI(rhi::ERHILogLevel level, const char *message)
    {
        if (gLoggerFunc)
        {
            gLoggerFunc(level, message);
        }
    };

    inline EColorFormat VkFormatToRHIFormat(VkFormat format)
    {
        switch (format)
        {
        case VK_FORMAT_B8G8R8A8_SRGB:
            return EColorFormat::BGRA8_UNorm;
        case VK_FORMAT_R8G8B8A8_SRGB:
            return EColorFormat::RGBA8_UNorm;
        default:
            return EColorFormat::Unknown;
        }
    }

    inline rhi::EQueueFeatures QueueTypeFromVkFlags(VkQueueFlags Flags)
    {
        rhi::EQueueFeatures Result = rhi::EQueueFeatures::None;

        if (Flags & VK_QUEUE_GRAPHICS_BIT)
        {
            Result |= rhi::EQueueFeatures::GRAPHICS;
        }
        if (Flags & VK_QUEUE_COMPUTE_BIT)
        {
            Result |= rhi::EQueueFeatures::COMPUTE;
        }
        if (Flags & VK_QUEUE_TRANSFER_BIT)
        {
            Result |= rhi::EQueueFeatures::TRANSFER;
        }
        if (Flags & VK_QUEUE_SPARSE_BINDING_BIT)
        {
            Result |= rhi::EQueueFeatures::SPARSE_BINDING;
        }
        if (Flags & VK_QUEUE_PROTECTED_BIT)
        {
            Result |= rhi::EQueueFeatures::PROTECTED;
        }
        if (Flags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)
        {
            Result |= rhi::EQueueFeatures::VIDEO_DECODE;
        }
        if (Flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)
        {
            Result |= rhi::EQueueFeatures::VIDEO_ENCODE;
        }
        if (Flags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)
        {
            Result |= rhi::EQueueFeatures::OPTICAL_FLOW;
        }
        if (Flags & VK_QUEUE_DATA_GRAPH_BIT_ARM)
        {
            Result |= rhi::EQueueFeatures::DATA_GRAPH_ARM;
        }

        return Result;
    }

    inline uint32_t GetQueueFamilyWithFeatures(const std::vector<FVkQueueFamilyInfo> &QueueFamilies, rhi::EQueueFeatures InFeatures)
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(QueueFamilies.size()); ++i)
        {
            if (QueueFamilies[i].SupportsFeatures(InFeatures))
            {
                return i;
            }
        }

        return UINT32_MAX;
    }

    inline void EnumerateQueueFamilies(VkPhysicalDevice InPhysicalDevice, std::vector<FVkQueueFamilyInfo> &OutQueueFamilies, VkSurfaceKHR InSurface = VK_NULL_HANDLE)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(InPhysicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);

        if (queueFamilyCount > 0)
        {
            vkGetPhysicalDeviceQueueFamilyProperties(InPhysicalDevice, &queueFamilyCount, queueFamilies.data());
        }

        OutQueueFamilies.resize(queueFamilyCount);

        for (int i = 0; i < static_cast<int>(queueFamilies.size()); ++i)
        {
            const VkQueueFamilyProperties &family = queueFamilies[i];
            rhi::EQueueFeatures type = QueueTypeFromVkFlags(family.queueFlags);
            OutQueueFamilies[i].Type = type;
            OutQueueFamilies[i].NumQueues = static_cast<int>(family.queueCount);
            OutQueueFamilies[i].FamilyIndex = i;
            if(InSurface != VK_NULL_HANDLE)
            {
                VkBool32 presentSupported = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(InPhysicalDevice, i, InSurface, &presentSupported);
                OutQueueFamilies[i].bSupportsPresent = presentSupported == VK_TRUE;
            }
        }
    }

    struct FSwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR Capabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    inline FSwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice InDevice, VkSurfaceKHR InSurface)
    {
        FSwapchainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(InDevice, InSurface, &details.Capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(InDevice, InSurface, &formatCount, nullptr);
        if (formatCount != 0)
        {
            details.Formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(InDevice, InSurface, &formatCount, details.Formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(InDevice, InSurface, &presentModeCount, nullptr);
        if (presentModeCount != 0)
        {
            details.PresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(InDevice, InSurface, &presentModeCount, details.PresentModes.data());
        }

        return details;
    }

    inline VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (const auto &availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB  && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    inline VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        for (const auto &availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    inline VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height)
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            return capabilities.currentExtent;
        }
        else
        {
            VkExtent2D actualExtent = {width, height};

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }
}