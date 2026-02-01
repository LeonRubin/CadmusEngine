#pragma once
#include "BitmaskTemplates.hpp"

namespace rhi::vulkan
{

    consteval void enable_bitmask_operators(EQueueFeatures);

    struct FVkQueueFamilyInfo
    {
        EQueueFeatures Type; // Bitmask of supported queue family features
        int NumQueues = 0;
        uint32_t FamilyIndex = 0;
        bool bSupportsPresent = false;
        
        bool SupportsFeatures(EQueueFeatures InFeatures) const
        {
            return (Type & InFeatures) == InFeatures;
        }
    };
}