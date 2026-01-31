#pragma once

#include <cstdint>

#include "Defs.hpp"

namespace rhi
{
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
