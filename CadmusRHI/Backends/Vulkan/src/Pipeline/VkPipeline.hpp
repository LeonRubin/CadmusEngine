#pragma once

#include "Defs.hpp"
#include "Types.hpp"
#include "Pipeline.hpp"

#include <vulkan/vulkan_core.h>

namespace rhi
{

    class VulkanPipeline : public IPipeline
    {
    public:
        VulkanPipeline(EPipelineType InType, VkDevice InDevice, VkPipelineLayout InLayout, VkPipeline InPipeline = VK_NULL_HANDLE)
            : Type(InType)
            , Device(InDevice)
            , Layout(InLayout)
            , PipelineHandle(InPipeline)
        {
        }

        ~VulkanPipeline() override
        {
            if (Device != VK_NULL_HANDLE)
            {
                if (PipelineHandle != VK_NULL_HANDLE)
                {
                    vkDestroyPipeline(Device, PipelineHandle, nullptr);
                }

                if (Layout != VK_NULL_HANDLE)
                {
                    vkDestroyPipelineLayout(Device, Layout, nullptr);
                }
            }
        }

        EPipelineType GetType() const override { return Type; }
    
        private:
        EPipelineType Type;
        VkDevice Device;
        VkPipelineLayout Layout;
        VkPipeline PipelineHandle;
    };
}