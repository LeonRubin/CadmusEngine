#include "VkAdapter.hpp"
#include <vulkan/vulkan.h>

#include "Helpers/VkHelper.hpp"

#include <utility>
#include <vector>
#include <algorithm>

namespace rhi::vulkan
{
    VkGraphicsAdapter::VkGraphicsAdapter(FGxAdapterInfo info, FVkAdapterCreateInfo& createInfo)
        : Info(std::move(info))
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(createInfo.NumQueueFamilies);
        float priorities[16] = {0.0f};

        for (size_t i = 0; i < createInfo.NumQueueFamilies; ++i)
        {
            VkDeviceQueueCreateInfo& queueCreateInfo = queueCreateInfos[i];
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = createInfo.QueueFamilyInfos[i].FamilyIndex;
            queueCreateInfo.queueCount = createInfo.NumQueuesPerFamily < static_cast<size_t>(createInfo.QueueFamilyInfos[i].NumQueues)
                                             ? static_cast<uint32_t>(createInfo.NumQueuesPerFamily)
                                             : static_cast<uint32_t>(createInfo.QueueFamilyInfos[i].NumQueues);
            queueCreateInfo.pQueuePriorities = &priorities[0];
        }
        
        VkPhysicalDeviceFeatures deviceFeatures{};

        std::vector<const char*> requiredDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

        VK_CHECK_RESULT(vkCreateDevice(createInfo.PhysicalDevice, &deviceCreateInfo, nullptr, &Device));
    }

    VkGraphicsAdapter::~VkGraphicsAdapter()
    {
        if (Device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(Device, nullptr);
            Device = VK_NULL_HANDLE;
        }
    }

    const FGxAdapterInfo& VkGraphicsAdapter::GetInfo() const
    {
        return Info;
    }
}
