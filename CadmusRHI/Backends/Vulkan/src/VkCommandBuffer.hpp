#pragma once

#include <CommandBuffer.hpp>
#include <vulkan/vulkan_core.h>

namespace rhi
{
    class IPipeline;
}

namespace rhi::vulkan
{
    class FVulkanContext;

    class FVulkanCommandBuffer final : public rhi::ICommandBuffer
    {
    public:
        FVulkanCommandBuffer(FVulkanContext* InVkContext, VkDevice InDevice, VkCommandPool InOwnerPool, VkCommandBuffer InCommandBuffer, rhi::EQueueFeatures InQueueFeatures, uint32_t InQueueFamilyIndex, VkQueue InQueue)
            : VkContext(InVkContext)
            , Device(InDevice)
            , OwnerPool(InOwnerPool)
            , CommandBuffer(InCommandBuffer)
            , QueueFeatures(InQueueFeatures)
            , QueueFamilyIndex(InQueueFamilyIndex)
            , Queue(InQueue)
        {
        }

        bool Begin(const rhi::FCommandBufferBeginDesc& Desc) override;
        bool End() override;

        void BeginRenderPass(const rhi::FRenderPassDesc& Desc) override;
        void EndRenderPass() override;

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;

        void BindPipeline(rhi::IPipeline* Pipeline) override;

        bool Reset() override;

        bool IsRecording() const override
        {
            return bRecording;
        }

        VkCommandBuffer GetVkCommandBuffer() const { return CommandBuffer; }
        VkCommandPool GetOwnerPool() const { return OwnerPool; }
        VkQueue GetQueue() const { return Queue; }
        uint32_t GetQueueFamilyIndex() const { return QueueFamilyIndex; }
        rhi::EQueueFeatures GetQueueFeatures() const { return QueueFeatures; }

    private:

        FVulkanContext* VkContext;
        VkDevice Device{VK_NULL_HANDLE};
        VkCommandPool OwnerPool{VK_NULL_HANDLE};
        VkCommandBuffer CommandBuffer{VK_NULL_HANDLE};
        rhi::EQueueFeatures QueueFeatures{rhi::EQueueFeatures::None};
        uint32_t QueueFamilyIndex{0};
        VkQueue Queue{VK_NULL_HANDLE};
        bool bRecording{false};

        bool bInRenderPass{false};
        FRenderPassDesc CurrentRenderPassDesc{};
    };
}