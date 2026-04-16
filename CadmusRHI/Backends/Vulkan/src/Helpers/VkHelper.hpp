#pragma once

#include <cassert>
#include <cstdio>
#include <algorithm>

#include <Context.hpp>
#include <vulkan/vulkan_core.h>
#include <Types.hpp>
#include <vector>

#include "Types.hpp"
#include <format>
#include <source_location>

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

    // Implement this sink with your current backend (console/file/debug output).
    inline void LogRHIWrite(ERHILogLevel level, std::string_view message, const std::source_location &where)
    {
        if (gLoggerFunc)
        {
            constexpr std::string_view kFmt = "[{}:{} {}] {}";
            std::string out;
            std::format_to(std::back_inserter(out), kFmt, where.file_name(), where.line(), where.function_name(), message);
            gLoggerFunc(level, out.c_str());
        }
    }

    inline void LogRHI(ERHILogLevel level,
                       std::string_view message,
                       const std::source_location &where = std::source_location::current())
    {
        LogRHIWrite(level, message, where);
    }

    // Compile-time checked format string overload.
    template <typename... TArgs>
    inline void LogRHI(ERHILogLevel level,
                       std::format_string<TArgs...> fmt,
                       TArgs &&...args)
    {
        LogRHIWrite(
            level,
            std::format(fmt, std::forward<TArgs>(args)...),
            std::source_location::current());
    }

    // Optional runtime format string overload (if needed).
    template <typename... TArgs>
    inline void LogRHI_RT(ERHILogLevel level,
                          std::string_view fmt,
                          TArgs &&...args)
    {
        LogRHIWrite(
            level,
            std::vformat(fmt, std::make_format_args(std::forward<TArgs>(args)...)),
            std::source_location::current());
    }

    inline EColorFormat VkFormatToRHIFormat(VkFormat format)
    {
        switch (format)
        {
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
            return EColorFormat::BGRA8_UNorm;
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
            return EColorFormat::RGBA8_UNorm;
        default:
            return EColorFormat::Unknown;
        }
    }

    inline VkFormat ConvertColorFormat(EColorFormat format)
    {
        switch (format)
        {
        case EColorFormat::RGBA8_UNorm:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case EColorFormat::BGRA8_UNorm:
            return VK_FORMAT_B8G8R8A8_UNORM;
        default:
            return VK_FORMAT_UNDEFINED;
        }
    }

    inline VkImageType ConvertImageType(ETextureType type)
    {
        switch (type)
        {
        case ETextureType::Texture2D:
            return VK_IMAGE_TYPE_2D;
        case ETextureType::Texture3D:
            return VK_IMAGE_TYPE_3D;
        default:
            return VK_IMAGE_TYPE_2D;
        }
    }

    inline VkImageViewType ConvertImageViewType(ETextureType type)
    {
        switch (type)
        {
        case ETextureType::Texture2D:
            return VK_IMAGE_VIEW_TYPE_2D;
        case ETextureType::Texture3D:
            return VK_IMAGE_VIEW_TYPE_3D;
        default:
            return VK_IMAGE_VIEW_TYPE_2D;
        }
    }

    inline VkImageUsageFlags ConvertTextureUsageFlags(ETextureUsageFlags usage)
    {
        VkImageUsageFlags usageFlags = 0;
        const uint32_t usageBits = static_cast<uint32_t>(usage);

        if ((usageBits & static_cast<uint32_t>(ETextureUsageFlags::Sampled)) != 0)
        {
            usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if ((usageBits & static_cast<uint32_t>(ETextureUsageFlags::Storage)) != 0)
        {
            usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if ((usageBits & static_cast<uint32_t>(ETextureUsageFlags::ColorAttachment)) != 0)
        {
            usageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        if ((usageBits & static_cast<uint32_t>(ETextureUsageFlags::DepthStencilAttachment)) != 0)
        {
            usageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        if ((usageBits & static_cast<uint32_t>(ETextureUsageFlags::TransferSrc)) != 0)
        {
            usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if ((usageBits & static_cast<uint32_t>(ETextureUsageFlags::TransferDst)) != 0)
        {
            usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

        return usageFlags;
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

    inline VkSampleCountFlagBits ConvertSampleCount(uint32_t SampleCount)
    {
        switch (SampleCount)
        {
        case 1:
            return VK_SAMPLE_COUNT_1_BIT;
        case 2:
            return VK_SAMPLE_COUNT_2_BIT;
        case 4:
            return VK_SAMPLE_COUNT_4_BIT;
        case 8:
            return VK_SAMPLE_COUNT_8_BIT;
        case 16:
            return VK_SAMPLE_COUNT_16_BIT;
        case 32:
            return VK_SAMPLE_COUNT_32_BIT;
        case 64:
            return VK_SAMPLE_COUNT_64_BIT;
        default:
            assert(false && "Unsupported sample count");
            return VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
        }
    }

    inline VkCullModeFlags ConvertCullMode(ECullMode CullMode)
    {
        switch (CullMode)
        {
        case ECullMode::None:
            return VK_CULL_MODE_NONE;
        case ECullMode::Front:
            return VK_CULL_MODE_FRONT_BIT;
        case ECullMode::Back:
            return VK_CULL_MODE_BACK_BIT;
        case ECullMode::FrontAndBack:
            return VK_CULL_MODE_FRONT_AND_BACK;
        default:
            assert(false && "Unsupported cull mode");
            return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
        }
    }

    inline VkPolygonMode ConvertPolygonMode(EPolygonMode Mode)
    {
        switch (Mode)
        {
        case EPolygonMode::Fill:
            return VK_POLYGON_MODE_FILL;
        case EPolygonMode::Line:
            return VK_POLYGON_MODE_LINE;
        case EPolygonMode::Point:
            return VK_POLYGON_MODE_POINT;
        default:
            assert(false && "Unsupported polygon mode");
            return VK_POLYGON_MODE_MAX_ENUM;
        }
    }

    inline VkPrimitiveTopology ConvertTopology(ETopology Topology)
    {
        switch (Topology)
        {
        case ETopology::PointList:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case ETopology::LineList:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case ETopology::LineStrip:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case ETopology::TriangleList:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case ETopology::TriangleStrip:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case ETopology::TriangleFan:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        default:
            assert(false && "Unsupported topology");
            return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
        }
    }

    inline VkFormat ConvertVertexFormat(EVertexFormat Format)
    {
        switch (Format)
        {
        case EVertexFormat::Float:
            return VK_FORMAT_R32_SFLOAT;
        case EVertexFormat::Float2:
            return VK_FORMAT_R32G32_SFLOAT;
        case EVertexFormat::Float3:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case EVertexFormat::Float4:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case EVertexFormat::UInt:
            return VK_FORMAT_R32_UINT;
        case EVertexFormat::UInt2:
            return VK_FORMAT_R32G32_UINT;
        case EVertexFormat::UInt3:
            return VK_FORMAT_R32G32B32_UINT;
        case EVertexFormat::UInt4:
            return VK_FORMAT_R32G32B32A32_UINT;
        case EVertexFormat::SInt:
            return VK_FORMAT_R32_SINT;
        case EVertexFormat::SInt2:
            return VK_FORMAT_R32G32_SINT;
        case EVertexFormat::SInt3:
            return VK_FORMAT_R32G32B32_SINT;
        case EVertexFormat::SInt4:
            return VK_FORMAT_R32G32B32A32_SINT;
        case EVertexFormat::UByte4Norm:
            return VK_FORMAT_R8G8B8A8_UNORM;
        default:
            assert(false && "Unsupported vertex format");
            return VK_FORMAT_UNDEFINED;
        }
    }

    inline VkCompareOp ConvertCompareOp(ECompareOp Op)
    {
        switch (Op)
        {
        case ECompareOp::Never:
            return VK_COMPARE_OP_NEVER;
        case ECompareOp::Less:
            return VK_COMPARE_OP_LESS;
        case ECompareOp::Equal:
            return VK_COMPARE_OP_EQUAL;
        case ECompareOp::LessOrEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case ECompareOp::Greater:
            return VK_COMPARE_OP_GREATER;
        case ECompareOp::NotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case ECompareOp::GreaterOrEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case ECompareOp::Always:
            return VK_COMPARE_OP_ALWAYS;
        default:
            assert(false && "Unsupported compare op");
            return VK_COMPARE_OP_MAX_ENUM;
        }
    }

    inline VkStencilOp ConvertStencilOp(EStencilOp Op)
    {
        switch (Op)
        {
        case EStencilOp::Keep:
            return VK_STENCIL_OP_KEEP;
        case EStencilOp::Zero:
            return VK_STENCIL_OP_ZERO;
        case EStencilOp::Replace:
            return VK_STENCIL_OP_REPLACE;
        case EStencilOp::IncrementAndClamp:
            return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case EStencilOp::DecrementAndClamp:
            return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case EStencilOp::Invert:
            return VK_STENCIL_OP_INVERT;
        case EStencilOp::IncrementAndWrap:
            return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case EStencilOp::DecrementAndWrap:
            return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        default:
            assert(false && "Unsupported stencil op");
            return VK_STENCIL_OP_MAX_ENUM;
        }
    }

    inline VkBlendFactor ConvertBlendFactor(EBlendFactor Factor)
    {
        switch (Factor)
        {
        case EBlendFactor::ZeroFactor:
            return VK_BLEND_FACTOR_ZERO;
        case EBlendFactor::OneFactor:
            return VK_BLEND_FACTOR_ONE;
        case EBlendFactor::SrcColor:
            return VK_BLEND_FACTOR_SRC_COLOR;
        case EBlendFactor::OneMinusSrcColor:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case EBlendFactor::DstColor:
            return VK_BLEND_FACTOR_DST_COLOR;
        case EBlendFactor::OneMinusDstColor:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case EBlendFactor::SrcAlpha:
            return VK_BLEND_FACTOR_SRC_ALPHA;
        case EBlendFactor::OneMinusSrcAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case EBlendFactor::DstAlpha:
            return VK_BLEND_FACTOR_DST_ALPHA;
        case EBlendFactor::OneMinusDstAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        default:
            assert(false && "Unsupported blend factor");
            return VK_BLEND_FACTOR_MAX_ENUM;
        }
    }

    inline VkBlendOp ConvertBlendOp(EBlendOp Op)
    {
        switch (Op)
        {
        case EBlendOp::Add:
            return VK_BLEND_OP_ADD;
        case EBlendOp::Subtract:
            return VK_BLEND_OP_SUBTRACT;
        case EBlendOp::ReverseSubtract:
            return VK_BLEND_OP_REVERSE_SUBTRACT;
        case EBlendOp::Min:
            return VK_BLEND_OP_MIN;
        case EBlendOp::Max:
            return VK_BLEND_OP_MAX;
        default:
            assert(false && "Unsupported blend op");
            return VK_BLEND_OP_MAX_ENUM;
        }
    }

    inline VkColorComponentFlags ConvertColorComponentFlags(EColorComponentFlags Flags)
    {
        VkColorComponentFlags Result = 0;
        if ((Flags & EColorComponentFlags::R) == EColorComponentFlags::R)
        {
            Result |= VK_COLOR_COMPONENT_R_BIT;
        }
        if ((Flags & EColorComponentFlags::G) == EColorComponentFlags::G)
        {
            Result |= VK_COLOR_COMPONENT_G_BIT;
        }
        if ((Flags & EColorComponentFlags::B) == EColorComponentFlags::B)
        {
            Result |= VK_COLOR_COMPONENT_B_BIT;
        }
        if ((Flags & EColorComponentFlags::A) == EColorComponentFlags::A)
        {
            Result |= VK_COLOR_COMPONENT_A_BIT;
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
            if (InSurface != VK_NULL_HANDLE)
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
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }

            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
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