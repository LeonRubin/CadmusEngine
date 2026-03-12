#include "ImmediateCommandsBufferManager.hpp"

#include "Helpers/VkHelper.hpp"
#include "VkContext.hpp"

#include <algorithm>

namespace
{
    bool SupportsFeatures(rhi::EQueueFeatures Available, rhi::EQueueFeatures Required)
    {
        return (Available & Required) == Required;
    }

    class FVulkanImmediateCommandBuffer final : public rhi::ICommandBuffer
    {
    public:
        FVulkanImmediateCommandBuffer(VkDevice InDevice, VkCommandPool InOwnerPool, VkCommandBuffer InCommandBuffer, rhi::EQueueFeatures InQueueFeatures, uint32_t InQueueFamilyIndex, VkQueue InQueue)
            : Device(InDevice)
            , OwnerPool(InOwnerPool)
            , CommandBuffer(InCommandBuffer)
            , QueueFeatures(InQueueFeatures)
            , QueueFamilyIndex(InQueueFamilyIndex)
            , Queue(InQueue)
        {
        }

        bool Begin(const rhi::FCommandBufferBeginDesc& Desc) override
        {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;

            if (Desc.OneTimeSubmit)
            {
                beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            }
            if (Desc.SimultaneousUse)
            {
                beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            }

            const VkResult result = vkBeginCommandBuffer(CommandBuffer, &beginInfo);
            bRecording = (result == VK_SUCCESS);
            return bRecording;
        }

        bool End() override
        {
            const VkResult result = vkEndCommandBuffer(CommandBuffer);
            if (result == VK_SUCCESS)
            {
                bRecording = false;
                return true;
            }

            return false;
        }

        bool Reset() override
        {
            const VkResult result = vkResetCommandBuffer(CommandBuffer, 0);
            if (result == VK_SUCCESS)
            {
                bRecording = false;
                return true;
            }

            return false;
        }

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
        VkDevice Device{VK_NULL_HANDLE};
        VkCommandPool OwnerPool{VK_NULL_HANDLE};
        VkCommandBuffer CommandBuffer{VK_NULL_HANDLE};
        rhi::EQueueFeatures QueueFeatures{rhi::EQueueFeatures::None};
        uint32_t QueueFamilyIndex{0};
        VkQueue Queue{VK_NULL_HANDLE};
        bool bRecording{false};
    };
}

using namespace rhi;

rhi::vulkan::ImmediateCommandsBufferManager::ImmediateCommandsBufferManager(IContext* InContext)
    : Context(InContext)
{
    auto* vulkanContext = dynamic_cast<FVulkanContext*>(Context);
    if (vulkanContext == nullptr)
    {
        return;
    }

    Device = vulkanContext->GetDevice();
}

rhi::vulkan::ImmediateCommandsBufferManager::~ImmediateCommandsBufferManager()
{
    if (Device == VK_NULL_HANDLE)
    {
        return;
    }

    for (const FImmediateSubmitInfo& submit : Submits)
    {
        if (submit.Fence != VK_NULL_HANDLE)
        {
            vkWaitForFences(Device, 1, &submit.Fence, VK_TRUE, UINT64_MAX);
        }
    }

    for (const FImmediateSubmitInfo& submit : Submits)
    {
        if (submit.Semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(Device, submit.Semaphore, nullptr);
        }
        if (submit.Fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(Device, submit.Fence, nullptr);
        }
    }

    for (VkSemaphore semaphore : SemaphorePool)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(Device, semaphore, nullptr);
        }
    }

    for (VkFence fence : FencePool)
    {
        if (fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(Device, fence, nullptr);
        }
    }

    for (const FCommandPoolEntry& poolEntry : CommandPools)
    {
        if (poolEntry.bOwned && poolEntry.Pool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(Device, poolEntry.Pool, nullptr);
        }
    }

    Submits.clear();
    SemaphorePool.clear();
    FencePool.clear();
    FreeCommandBuffers.clear();
    OwnedCommandBuffers.clear();
    CommandPools.clear();
}

ICommandBuffer *rhi::vulkan::ImmediateCommandsBufferManager::Acquire(EQueueFeatures RequiredFeatures)
{
    if (Device == VK_NULL_HANDLE)
    {
        return nullptr;
    }

    CollectFinishedSubmits();

    FCommandPoolEntry* targetPool = FindPoolWithFeatures(RequiredFeatures);
    if (targetPool == nullptr)
    {
        targetPool = &CreatePoolForFeatures(RequiredFeatures);
    }

    if (targetPool == nullptr || targetPool->Pool == VK_NULL_HANDLE || targetPool->Queue == VK_NULL_HANDLE)
    {
        LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Failed to acquire command pool for immediate command buffer");
        
        return nullptr;
    }

    auto reusableIt = std::find_if(FreeCommandBuffers.begin(), FreeCommandBuffers.end(), [targetPool, RequiredFeatures](ICommandBuffer* buffer)
    {
        auto* vkBuffer = dynamic_cast<FVulkanImmediateCommandBuffer*>(buffer);
        if (vkBuffer == nullptr)
        {
            return false;
        }

        return vkBuffer->GetOwnerPool() == targetPool->Pool
            && vkBuffer->GetQueueFamilyIndex() == targetPool->QueueFamilyIndex
            && SupportsFeatures(vkBuffer->GetQueueFeatures(), RequiredFeatures);
    });

    if (reusableIt != FreeCommandBuffers.end())
    {
        ICommandBuffer* commandBuffer = *reusableIt;
        FreeCommandBuffers.erase(reusableIt);
        commandBuffer->Reset();
        return commandBuffer;
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = targetPool->Pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer vkCommandBuffer = VK_NULL_HANDLE;
    const VkResult allocRes = vkAllocateCommandBuffers(Device, &allocInfo, &vkCommandBuffer);
    if (allocRes != VK_SUCCESS)
    {
        return nullptr;
    }

    auto buffer = std::make_unique<FVulkanImmediateCommandBuffer>(
        Device,
        targetPool->Pool,
        vkCommandBuffer,
        targetPool->QueueFeatures,
        targetPool->QueueFamilyIndex,
        targetPool->Queue);

    ICommandBuffer* bufferPtr = buffer.get();
    OwnedCommandBuffers.emplace_back(std::move(buffer));

    return bufferPtr;
}

bool rhi::vulkan::ImmediateCommandsBufferManager::Submit(const ICommandBuffer* pCommandBuffers, size_t CommandBufferCount, bool bSyncWithPreviousSubmits)
{
    if (Device == VK_NULL_HANDLE || pCommandBuffers == nullptr || CommandBufferCount == 0)
    {
        return false;
    }

    CollectFinishedSubmits();

    std::vector<VkCommandBuffer> vkBuffers;
    vkBuffers.reserve(CommandBufferCount);

    std::vector<ICommandBuffer*> submittedBuffers;
    submittedBuffers.reserve(CommandBufferCount);

    VkQueue submitQueue = VK_NULL_HANDLE;
    EQueueFeatures queueFeatures = EQueueFeatures::None;

    for (size_t index = 0; index < CommandBufferCount; ++index)
    {
        const auto* commandBuffer = dynamic_cast<const FVulkanImmediateCommandBuffer*>(&pCommandBuffers[index]);
        if (commandBuffer == nullptr)
        {
            return false;
        }

        if (submitQueue == VK_NULL_HANDLE)
        {
            submitQueue = commandBuffer->GetQueue();
            queueFeatures = commandBuffer->GetQueueFeatures();
        }
        else if (submitQueue != commandBuffer->GetQueue())
        {
            return false;
        }

        vkBuffers.push_back(commandBuffer->GetVkCommandBuffer());
        submittedBuffers.push_back(const_cast<FVulkanImmediateCommandBuffer*>(commandBuffer));
    }

    if (submitQueue == VK_NULL_HANDLE)
    {
        return false;
    }

    while (static_cast<int>(Submits.size()) >= kMaxActiveSubmissions)
    {
        FImmediateSubmitInfo& oldestSubmit = Submits.front();
        if (oldestSubmit.Fence != VK_NULL_HANDLE)
        {
            vkWaitForFences(Device, 1, &oldestSubmit.Fence, VK_TRUE, UINT64_MAX);
        }

        for (ICommandBuffer* cmdBuffer : oldestSubmit.CommandBuffers)
        {
            if (cmdBuffer != nullptr)
            {
                cmdBuffer->Reset();
                FreeCommandBuffers.push_back(cmdBuffer);
            }
        }

        RecycleSyncObjects(oldestSubmit.Fence, oldestSubmit.Semaphore);
        Submits.pop_front();
    }

    VkFence fence = AcquireFence();
    VkSemaphore signalSemaphore = AcquireSemaphore();

    if (fence == VK_NULL_HANDLE || signalSemaphore == VK_NULL_HANDLE)
    {
        RecycleSyncObjects(fence, signalSemaphore);
        return false;
    }

    VK_CHECK_RESULT(vkResetFences(Device, 1, &fence));

    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSemaphore waitSemaphore = VK_NULL_HANDLE;

    if (bSyncWithPreviousSubmits && !Submits.empty())
    {
        waitSemaphore = Submits.back().Semaphore;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = static_cast<uint32_t>(vkBuffers.size());
    submitInfo.pCommandBuffers = vkBuffers.data();
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &signalSemaphore;

    if (waitSemaphore != VK_NULL_HANDLE)
    {
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.pWaitDstStageMask = &waitStageMask;
    }

    const VkResult submitResult = vkQueueSubmit(submitQueue, 1, &submitInfo, fence);
    if (submitResult != VK_SUCCESS)
    {
        RecycleSyncObjects(fence, signalSemaphore);
        return false;
    }

    Submits.push_back(FImmediateSubmitInfo{
        .Fence = fence,
        .Semaphore = signalSemaphore,
        .QueueFeatures = queueFeatures,
        .CommandBuffers = std::move(submittedBuffers)});

    return true;
}

void rhi::vulkan::ImmediateCommandsBufferManager::Reset()
{
    CollectFinishedSubmits();

    for (FImmediateSubmitInfo& submit : Submits)
    {
        if (submit.Fence != VK_NULL_HANDLE)
        {
            vkWaitForFences(Device, 1, &submit.Fence, VK_TRUE, UINT64_MAX);
        }

        for (ICommandBuffer* cmdBuffer : submit.CommandBuffers)
        {
            if (cmdBuffer != nullptr)
            {
                cmdBuffer->Reset();
                FreeCommandBuffers.push_back(cmdBuffer);
            }
        }

        RecycleSyncObjects(submit.Fence, submit.Semaphore);
    }
    Submits.clear();

    for (ICommandBuffer* commandBuffer : FreeCommandBuffers)
    {
        if (commandBuffer != nullptr)
        {
            commandBuffer->Reset();
        }
    }

    for (const FCommandPoolEntry& poolEntry : CommandPools)
    {
        if (poolEntry.Pool != VK_NULL_HANDLE)
        {
            VK_CHECK_RESULT(vkResetCommandPool(Device, poolEntry.Pool, 0));
        }
    }
}

rhi::vulkan::FCommandPoolEntry* rhi::vulkan::ImmediateCommandsBufferManager::FindPoolWithFeatures(EQueueFeatures RequiredFeatures)
{
    auto it = std::find_if(CommandPools.begin(), CommandPools.end(), [RequiredFeatures](const FCommandPoolEntry& poolEntry)
    {
        return poolEntry.Pool != VK_NULL_HANDLE && SupportsFeatures(poolEntry.QueueFeatures, RequiredFeatures);
    });

    if (it == CommandPools.end())
    {
        return nullptr;
    }

    return &(*it);
}

rhi::vulkan::FCommandPoolEntry& rhi::vulkan::ImmediateCommandsBufferManager::CreatePoolForFeatures(EQueueFeatures RequiredFeatures)
{
    auto* vulkanContext = static_cast<FVulkanContext*>(Context);
    if (vulkanContext == nullptr)
    {
        CommandPools.push_back(FCommandPoolEntry{});
        return CommandPools.back();
    }

    VkCommandPool commandPool = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex = 0;
    EQueueFeatures queueFeatures = EQueueFeatures::None;
    VkQueue queue = VK_NULL_HANDLE;

    const bool acquired = vulkanContext->AcquireImmediateCommandPool(
        RequiredFeatures,
        commandPool,
        queueFamilyIndex,
        queue,
        queueFeatures);

    if (!acquired || commandPool == VK_NULL_HANDLE)
    {
        CommandPools.push_back(FCommandPoolEntry{});
        return CommandPools.back();
    }

    auto existingIt = std::find_if(CommandPools.begin(), CommandPools.end(), [commandPool](const FCommandPoolEntry& entry)
    {
        return entry.Pool == commandPool;
    });

    if (existingIt != CommandPools.end())
    {
        return *existingIt;
    }

    CommandPools.push_back(FCommandPoolEntry{
        .Pool = commandPool,
        .Queue = queue,
        .QueueFamilyIndex = queueFamilyIndex,
        .QueueFeatures = queueFeatures,
        .bOwned = false});

    return CommandPools.back();
}

void rhi::vulkan::ImmediateCommandsBufferManager::CollectFinishedSubmits()
{
    for (auto it = Submits.end(); it != Submits.begin();)
    {
        --it;
        if (it->Fence == VK_NULL_HANDLE)
        {
            continue;
        }

        const VkResult fenceStatus = vkGetFenceStatus(Device, it->Fence);
        if (fenceStatus != VK_SUCCESS)
        {
            continue;
        }

        for (ICommandBuffer* cmdBuffer : it->CommandBuffers)
        {
            if (cmdBuffer != nullptr)
            {
                cmdBuffer->Reset();
                FreeCommandBuffers.push_back(cmdBuffer);
            }
        }

        RecycleSyncObjects(it->Fence, it->Semaphore);
        it = Submits.erase(it);
    }
}

VkFence rhi::vulkan::ImmediateCommandsBufferManager::AcquireFence()
{
    if (!FencePool.empty())
    {
        VkFence fence = FencePool.back();
        FencePool.pop_back();
        return fence;
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = 0;

    VkFence fence = VK_NULL_HANDLE;
    const VkResult result = vkCreateFence(Device, &fenceInfo, nullptr, &fence);
    if (result != VK_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }

    return fence;
}

VkSemaphore rhi::vulkan::ImmediateCommandsBufferManager::AcquireSemaphore()
{
    if (!SemaphorePool.empty())
    {
        VkSemaphore semaphore = SemaphorePool.back();
        SemaphorePool.pop_back();
        return semaphore;
    }

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkSemaphore semaphore = VK_NULL_HANDLE;
    const VkResult result = vkCreateSemaphore(Device, &semaphoreInfo, nullptr, &semaphore);
    if (result != VK_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }

    return semaphore;
}

void rhi::vulkan::ImmediateCommandsBufferManager::RecycleSyncObjects(VkFence Fence, VkSemaphore Semaphore)
{
    if (Fence != VK_NULL_HANDLE)
    {
        FencePool.push_back(Fence);
    }

    if (Semaphore != VK_NULL_HANDLE)
    {
        SemaphorePool.push_back(Semaphore);
    }
}
