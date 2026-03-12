#include "VkAdapter.hpp"
#include <vulkan/vulkan.h>

#include "Helpers/VkHelper.hpp"

#include <utility>
#include <vector>
#include <algorithm>
#include <cstring>

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

        uint32_t availableExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties(createInfo.PhysicalDevice, nullptr, &availableExtensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
        if (availableExtensionCount > 0)
        {
            vkEnumerateDeviceExtensionProperties(createInfo.PhysicalDevice, nullptr, &availableExtensionCount, availableExtensions.data());
        }

        auto hasDeviceExtension = [&availableExtensions](const char* extensionName) -> bool
        {
            return std::any_of(availableExtensions.begin(), availableExtensions.end(), [extensionName](const VkExtensionProperties& ext)
            {
                return std::strcmp(ext.extensionName, extensionName) == 0;
            });
        };

        std::vector<const char*> requiredDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
        };

        const bool hasExtDynState = hasDeviceExtension(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
        const bool hasExtDynState2 = hasDeviceExtension(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
        const bool hasExtDynState3 = hasDeviceExtension(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
        const bool hasDynamicRendering = hasDeviceExtension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

        if (!hasDynamicRendering)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Required Vulkan device extension VK_KHR_dynamic_rendering is not supported.");
            assert(false && "VK_KHR_dynamic_rendering is required");
        }

        if (hasExtDynState)
        {
            requiredDeviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
        }
        if (hasExtDynState2)
        {
            requiredDeviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
        }
        if (hasExtDynState3)
        {
            requiredDeviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
        }

        VkPhysicalDeviceFeatures2 supportedFeatures2{};
        supportedFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT supportedExtendedDynamicState{};
        supportedExtendedDynamicState.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;

        VkPhysicalDeviceExtendedDynamicState2FeaturesEXT supportedExtendedDynamicState2{};
        supportedExtendedDynamicState2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;

        VkPhysicalDeviceExtendedDynamicState3FeaturesEXT supportedExtendedDynamicState3{};
        supportedExtendedDynamicState3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;

        VkPhysicalDeviceDynamicRenderingFeaturesKHR supportedDynamicRendering{};
        supportedDynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;

        supportedFeatures2.pNext = &supportedExtendedDynamicState;
        supportedExtendedDynamicState.pNext = &supportedExtendedDynamicState2;
        supportedExtendedDynamicState2.pNext = &supportedExtendedDynamicState3;
        supportedExtendedDynamicState3.pNext = &supportedDynamicRendering;

        vkGetPhysicalDeviceFeatures2(createInfo.PhysicalDevice, &supportedFeatures2);

        VkPhysicalDeviceFeatures2 enabledFeatures2{};
        enabledFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        enabledFeatures2.features = deviceFeatures;

        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT enabledExtendedDynamicState{};
        enabledExtendedDynamicState.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;

        VkPhysicalDeviceExtendedDynamicState2FeaturesEXT enabledExtendedDynamicState2{};
        enabledExtendedDynamicState2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;

        VkPhysicalDeviceExtendedDynamicState3FeaturesEXT enabledExtendedDynamicState3{};
        enabledExtendedDynamicState3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;

        VkPhysicalDeviceDynamicRenderingFeaturesKHR enabledDynamicRendering{};
        enabledDynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;

        enabledFeatures2.pNext = &enabledExtendedDynamicState;
        enabledExtendedDynamicState.pNext = &enabledExtendedDynamicState2;
        enabledExtendedDynamicState2.pNext = &enabledExtendedDynamicState3;
        enabledExtendedDynamicState3.pNext = &enabledDynamicRendering;

        if (hasExtDynState)
        {
            enabledExtendedDynamicState.extendedDynamicState = supportedExtendedDynamicState.extendedDynamicState;
        }

        if (hasExtDynState2)
        {
            enabledExtendedDynamicState2.extendedDynamicState2 = supportedExtendedDynamicState2.extendedDynamicState2;
            enabledExtendedDynamicState2.extendedDynamicState2LogicOp = supportedExtendedDynamicState2.extendedDynamicState2LogicOp;
            enabledExtendedDynamicState2.extendedDynamicState2PatchControlPoints = supportedExtendedDynamicState2.extendedDynamicState2PatchControlPoints;
        }

        if (hasExtDynState3)
        {
            enabledExtendedDynamicState3.extendedDynamicState3DepthClampEnable = supportedExtendedDynamicState3.extendedDynamicState3DepthClampEnable;
            enabledExtendedDynamicState3.extendedDynamicState3PolygonMode = supportedExtendedDynamicState3.extendedDynamicState3PolygonMode;
            enabledExtendedDynamicState3.extendedDynamicState3RasterizationSamples = supportedExtendedDynamicState3.extendedDynamicState3RasterizationSamples;
            enabledExtendedDynamicState3.extendedDynamicState3SampleMask = supportedExtendedDynamicState3.extendedDynamicState3SampleMask;
            enabledExtendedDynamicState3.extendedDynamicState3AlphaToCoverageEnable = supportedExtendedDynamicState3.extendedDynamicState3AlphaToCoverageEnable;
            enabledExtendedDynamicState3.extendedDynamicState3AlphaToOneEnable = supportedExtendedDynamicState3.extendedDynamicState3AlphaToOneEnable;
        }

        if (supportedDynamicRendering.dynamicRendering != VK_TRUE)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Vulkan device does not support dynamicRendering feature.");
            assert(false && "dynamicRendering feature is required");
        }

        enabledDynamicRendering.dynamicRendering = supportedDynamicRendering.dynamicRendering;

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = &enabledFeatures2;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = nullptr;
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
