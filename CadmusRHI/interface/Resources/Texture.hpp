#pragma once

#include "../Types.hpp"
#include "../BitmaskTemplates.hpp"

namespace rhi
{
    struct FTextureCreateInfo
    {
        ETextureType Type{ETextureType::Texture2D};
        EColorFormat Format{EColorFormat::Unknown};
        FExtent3D Extent{1, 1, 1};
        uint32_t MipLevels{1};
        uint32_t ArrayLayers{1};
        uint32_t SampleCount{1};
        ETextureUsageFlags Usage{ETextureUsageFlags::Sampled};

        constexpr FTextureCreateInfo() = default;

        constexpr FTextureCreateInfo(ETextureType InType, EColorFormat InFormat, FExtent3D InExtent, uint32_t InMipLevels = 1, uint32_t InArrayLayers = 1, uint32_t InSampleCount = 1, ETextureUsageFlags InUsage = ETextureUsageFlags::Sampled)
            : Type(InType), Format(InFormat), Extent(InExtent), MipLevels(InMipLevels), ArrayLayers(InArrayLayers), SampleCount(InSampleCount), Usage(InUsage)
        {
        }

        static constexpr FTextureCreateInfo Texture2D(EColorFormat InFormat, FExtent2D InExtent, uint32_t InMipLevels = 1, uint32_t InArrayLayers = 1, uint32_t InSampleCount = 1, ETextureUsageFlags InUsage = ETextureUsageFlags::Sampled)
        {
            return FTextureCreateInfo{
                ETextureType::Texture2D,
                InFormat,
                FExtent3D{InExtent.Width, InExtent.Height, 1},
                InMipLevels,
                InArrayLayers,
                InSampleCount,
                InUsage};
        }

        static constexpr FTextureCreateInfo Texture3D(EColorFormat InFormat, FExtent3D InExtent, uint32_t InMipLevels = 1, ETextureUsageFlags InUsage = ETextureUsageFlags::Sampled)
        {
            return FTextureCreateInfo{
                ETextureType::Texture3D,
                InFormat,
                InExtent,
                InMipLevels,
                1,
                1,
                InUsage};
        }
    };

    class CADMUS_RHI_API ITexture
    {
    public:
        ITexture() = default;
        virtual ~ITexture() = default;

        const FTextureCreateInfo &GetCreateInfo() const
        {
            return CreateInfo;
        };

    protected:
        FTextureCreateInfo CreateInfo;
    };
}