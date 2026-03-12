#pragma once

#include "VkBackend.hpp"
#include "VkSwapchain.hpp"
#include "Helpers/Types.hpp"
#include "VkAdapter.hpp"

#define VK_MAX_QUEUE_FAMILIES_PER_TYPE 8

namespace rhi::vulkan
{
    struct FVkImmediatePoolEntry
    {
        VkCommandPool Pool{VK_NULL_HANDLE};
        uint32_t QueueFamilyIndex{0};
        EQueueFeatures QueueFeatures{EQueueFeatures::None};
    };

    class ImmediateCommandsBufferManager;
    class FVulkanContext : public IContext
    {
    public:
        FVulkanContext();
        ~FVulkanContext() override;

        bool Initialize(const FContextInitParams &Params) override;
        IPipelineBuilder* GetPipelineBuilder() override;
        ICommandBuffersPool* GetCommandBuffersPool(EQueueFeatures RequiredFeatures) override;
        ICommandBuffer* AcquireImmediateCommandBuffer(EQueueFeatures RequiredFeatures) override;

        VkDevice GetDevice() const;
        bool AcquireImmediateCommandPool(EQueueFeatures RequiredFeatures, VkCommandPool& OutPool, uint32_t& OutQueueFamilyIndex, VkQueue& OutQueue, EQueueFeatures& OutQueueFeatures);

    private:
        uint32_t SelectPhysicalDevice(const std::vector<VkPhysicalDevice> &physicalDevices, int desiredIdx = -1);
        void ReleaseInstance();
        bool CreateInstance();
        void EnumerateAdapters();
        bool FindQueueFamilyForFeatures(EQueueFeatures RequiredFeatures, uint32_t& OutQueueFamilyIndex, EQueueFeatures& OutQueueFeatures) const;

        VkInstance Instance{VK_NULL_HANDLE};
        VkPhysicalDevice PhysicalDevice{VK_NULL_HANDLE};

        VkSurfaceKHR Surface{VK_NULL_HANDLE};
        std::vector<FGxAdapterInfo> AdapterInfos;
        std::vector<FVkQueueFamilyInfo> ActiveQueueFamilyInfos;
        std::vector<FVkImmediatePoolEntry> ImmediatePools;

        VkGraphicsAdapter *SelectedAdapter{nullptr};
        VkSwapchain *Swapchain{nullptr};

        ImmediateCommandsBufferManager* ImmediateCmdBufferManager = nullptr;

    };
}
