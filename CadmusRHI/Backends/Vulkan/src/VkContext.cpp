#include "VkContext.hpp"

#include <vector>
#include "Helpers/VkHelper.hpp"

namespace rhi::vulkan
{
    FVulkanContext::FVulkanContext() = default;

    FVulkanContext::~FVulkanContext()
    {
        ReleaseInstance();
    }

    bool FVulkanContext::Initialize(const FContextInitParams& Params)
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "CadmusEngine";
        appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
        appInfo.pEngineName = "CadmusEngine";
        appInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;


        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());


        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(Params.NumRequiredInstanceExtensions);
        createInfo.ppEnabledExtensionNames = Params.RequiredInstanceExtensions;

        VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &Instance));

        LogRHI(RHI_LOGLEVEL_INFO, "Vulkan instance created successfully.");

        EnumerateAdapters();
        return true;
    }

    bool FVulkanContext::CreateInstance()
    {

        return VK_SUCCESS;
    }

    void FVulkanContext::EnumerateAdapters()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr);
        if(deviceCount == 0)
        {
            return;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(Instance, &deviceCount, devices.data());

        for(VkPhysicalDevice device : devices)
        {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(device, &properties);

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            if(queueFamilyCount > 0)
            {
                vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
            }

            FQueueCounts queueCounts{};
            for(const VkQueueFamilyProperties& family : queueFamilies)
            {
                if(family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    queueCounts.Graphics++;
                }
                if(family.queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                    queueCounts.Compute++;
                }
                if(family.queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    queueCounts.Transfer++;
                }
            }
            queueCounts.Present = queueCounts.Graphics;

            const bool isDiscrete = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
            FGxAdapterInfo info(properties.deviceName, properties.vendorID, properties.deviceID, isDiscrete, queueCounts);

            Adapters.emplace_back(std::make_unique<FVulkanGxAdapter>(std::move(info)));
        }
    }

    void FVulkanContext::ReleaseInstance()
    {
        if(Instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(Instance, nullptr);
            Instance = VK_NULL_HANDLE;
        }
    }
}
