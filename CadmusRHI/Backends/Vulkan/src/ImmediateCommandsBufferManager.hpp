#pragma once

#include <Context.hpp>
#include <CommandBuffersPool.hpp>
#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <deque>
#include <memory>
#include <vector>

namespace rhi::vulkan
{
    struct FImmediateSubmitInfo
    {
        VkFence Fence{VK_NULL_HANDLE};
        VkSemaphore Semaphore{VK_NULL_HANDLE};
        EQueueFeatures QueueFeatures{EQueueFeatures::None};
        std::vector<ICommandBuffer*> CommandBuffers{};
    };

    struct FCommandPoolEntry
    {
        VkCommandPool Pool{VK_NULL_HANDLE};
        VkQueue Queue{VK_NULL_HANDLE};
        uint32_t QueueFamilyIndex{0};
        EQueueFeatures QueueFeatures{EQueueFeatures::None};
        bool bOwned{false};
    };

    class ImmediateCommandsBufferManager
    {
    public:
        explicit ImmediateCommandsBufferManager(IContext* InContext);
        ~ImmediateCommandsBufferManager();

        ICommandBuffer* Acquire(EQueueFeatures RequiredFeatures = EQueueFeatures::GRAPHICS);
        bool Submit(const ICommandBuffer* pCommandBuffers, size_t CommandBufferCount, bool bSyncWithPreviousSubmits = true);
        
        void Reset();

    private:
        const int kMaxActiveSubmissions = 512;

        FCommandPoolEntry* FindPoolWithFeatures(EQueueFeatures RequiredFeatures);
        FCommandPoolEntry& CreatePoolForFeatures(EQueueFeatures RequiredFeatures);
        void CollectFinishedSubmits();

        VkFence AcquireFence();
        VkSemaphore AcquireSemaphore();
        void RecycleSyncObjects(VkFence Fence, VkSemaphore Semaphore);
        
        IContext* Context{nullptr};
        VkDevice Device{VK_NULL_HANDLE};
        std::vector<FCommandPoolEntry> CommandPools;

        std::vector<std::unique_ptr<ICommandBuffer>> OwnedCommandBuffers;
        std::vector<ICommandBuffer*> FreeCommandBuffers;
        std::deque<FImmediateSubmitInfo> Submits;

        std::vector<VkFence> FencePool;
        std::vector<VkSemaphore> SemaphorePool;
    };
}