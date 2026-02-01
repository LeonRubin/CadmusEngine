#pragma once

#include <GxAdapter.hpp>
#include <vulkan/vulkan_core.h>
#include "Helpers/Types.hpp"

namespace rhi::vulkan
{
    struct FVkAdapterCreateInfo
    {
        VkPhysicalDevice PhysicalDevice{VK_NULL_HANDLE};
        FVkQueueFamilyInfo* QueueFamilyInfos{nullptr};
        size_t NumQueueFamilies{0};
        size_t NumQueuesPerFamily{1}; // Do we need more than one per specific queue family?
    };

    class VkGraphicsAdapter : public IGxAdapter
    {
    public:
        explicit VkGraphicsAdapter(FGxAdapterInfo info, FVkAdapterCreateInfo& createInfo);
        ~VkGraphicsAdapter() override;

        const FGxAdapterInfo& GetInfo() const override;

        VkDevice GetDevice() const { return Device; }
    private:
        FGxAdapterInfo Info;
        VkDevice Device{VK_NULL_HANDLE};
    };
}