#pragma once

#include <cstdint>

#include "Defs.hpp"
#include "BitmaskTemplates.hpp"

namespace rhi
{
    enum class EQueueFeatures : uint32_t
    {
        None = 0,
        GRAPHICS = 1u << 0,
        COMPUTE = 1u << 1,
        TRANSFER = 1u << 2,
        PRESENT = 1u << 3,
        SPARSE_BINDING = 1u << 4,
        PROTECTED = 1u << 5,
        VIDEO_DECODE = 1u << 6,
        VIDEO_ENCODE = 1u << 7,
        OPTICAL_FLOW = 1u << 8,
        DATA_GRAPH_ARM = 1u << 9,
        MAX_ENUM = 10,
        Any = 0xFFFFFFFF
    };

    consteval void enable_bitmask_operators(EQueueFeatures);

    struct CADMUS_RHI_API FExtent2D
    {
        uint32_t Width{0};
        uint32_t Height{0};

        constexpr FExtent2D() = default;
        constexpr FExtent2D(uint32_t InWidth, uint32_t InHeight)
            : Width(InWidth)
            , Height(InHeight)
        {
        }
    };

    enum class EColorFormat
    {
        Unknown,
        RGBA8_UNorm,
        BGRA8_UNorm
    };

    struct CADMUS_RHI_API FQueueCounts
    {
        uint32_t Graphics{0};
        uint32_t Compute{0};
        uint32_t Transfer{0};
        uint32_t Present{0};

        constexpr FQueueCounts() = default;
        constexpr FQueueCounts(uint32_t InGraphics, uint32_t InCompute, uint32_t InTransfer, uint32_t InPresent)
            : Graphics(InGraphics)
            , Compute(InCompute)
            , Transfer(InTransfer)
            , Present(InPresent)
        {
        }
    };
}
