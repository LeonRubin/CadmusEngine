#include "VkContext.hpp"

#include <vector>
#include "Helpers/VkHelper.hpp"
#include <algorithm>
#include <map>

namespace rhi::vulkan
{
    const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"};
    VkDebugUtilsMessengerEXT gDebugMessenger;
    
    FVulkanContext::FVulkanContext() = default;

    FVulkanContext::~FVulkanContext()
    {
        ReleaseInstance();
    }
    

    bool checkValidationLayerSupport()
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* requested : validationLayers)
        {
            const bool found = std::any_of(availableLayers.begin(), availableLayers.end(),
                [requested](const VkLayerProperties& props)
                {
                    return std::strcmp(requested, props.layerName) == 0;
                });

            if (!found)
            {
                char buf[256];
                std::snprintf(buf, sizeof(buf), "Validation layer missing: %s", requested);
                LogRHI(rhi::RHI_LOGLEVEL_ERROR, buf);
                return false;
            }
        }
        return true;
    }

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(instance, debugMessenger, pAllocator);
        }
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData)
    {
        if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT && messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            LogRHI(rhi::RHI_LOGLEVEL_WARNING, pCallbackData->pMessage);
        }
        else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, pCallbackData->pMessage);
        }
        else
        {
            LogRHI(rhi::RHI_LOGLEVEL_INFO, pCallbackData->pMessage);
        }

        return VK_FALSE;
    }

    uint32_t selectPhysicalDevice(const std::vector<VkPhysicalDevice>& physicalDevices, int desiredIdx = -1)
    {
        VkPhysicalDeviceProperties properties{};
        VkPhysicalDeviceFeatures features{};

        std::multimap<int, int> candidates;

        for(int i = 0; i < static_cast<int>(physicalDevices.size()); ++i)
        {
            vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);
            vkGetPhysicalDeviceFeatures(physicalDevices[i], &features);

            int score = 0;

            bool bCompatible = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
            bCompatible &= features.geometryShader == VK_TRUE;

            score += properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 10 : 0;
            score += properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? 5 : 0;
            
            candidates.insert({score, i});

            if(bCompatible && i == desiredIdx && desiredIdx != -1)
            {
                return i;
            }
        }

        if(candidates.begin()->first > 0)
        {
            return candidates.rbegin()->second;
        }else
        {
            return UINT32_MAX;
        }
    }

    bool FVulkanContext::Initialize(const FContextInitParams &Params)
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

        for (const auto &ext : availableExtensions)
        {
            char buf[256];
            std::snprintf(buf, sizeof(buf), "Available Vulkan Extension: %s (version %u)", ext.extensionName, ext.specVersion);
            LogRHI(rhi::RHI_LOGLEVEL_INFO, buf);
        }

        std::vector<const char *> requiredInstanceExtensions;
        std::vector<const char *> optionalInstanceExtensions;

        for (int i = 0; i < Params.NumRequiredInstanceExtensions; ++i)
        {
            requiredInstanceExtensions.push_back(Params.RequiredInstanceExtensions[i]);
        }

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

        if (Params.EnableValidationLayers && !checkValidationLayerSupport())
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Validation layers requested, but not available!");
            return false;
        }
        else
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            if (Params.EnableNativeLogging)
            {
                debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                debugCreateInfo.pfnUserCallback = debugCallback;
                
                createInfo.pNext = &debugCreateInfo;
            }
        }

        for (const char *requiredExt : requiredInstanceExtensions)
        {
            bool bFoundExt = false;
            for (const auto &ext : availableExtensions)
            {
                if (strcmp(requiredExt, ext.extensionName) == 0)
                {
                    bFoundExt = true;
                    break;
                }
            }

            if (!bFoundExt)
            {
                char buf[256];
                std::snprintf(buf, sizeof(buf), "Required Vulkan Instance Extension not available: %s", requiredExt);
                LogRHI(rhi::RHI_LOGLEVEL_ERROR, buf);
                return false;
            }
        }

        // Add optional extensions if available
        for (const char *optionalExt : optionalInstanceExtensions)
        {
            for (const auto &ext : availableExtensions)
            {
                if (strcmp(optionalExt, ext.extensionName) == 0)
                {
                    requiredInstanceExtensions.push_back(optionalExt);
                    break;
                }
            }
        }

        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredInstanceExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();
        VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &Instance));

        if (Params.EnableNativeLogging)
        {
            debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugCreateInfo.pfnUserCallback = debugCallback;
            debugCreateInfo.pUserData = nullptr;

            CreateDebugUtilsMessengerEXT(Instance, &debugCreateInfo, nullptr, &gDebugMessenger);
        }

        LogRHI(rhi::RHI_LOGLEVEL_INFO, "Vulkan instance created successfully.");

        EnumerateAdapters();

        if( AdapterInfos.empty())
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "No Vulkan physical devices found.");
            return false;
        }

        uint32_t deviceCount = AdapterInfos.size();
        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(Instance, &deviceCount, physicalDevices.data());

        auto physicalDeviceID = selectPhysicalDevice(physicalDevices, Params.DesiredDeviceIdx);
        if(physicalDeviceID == UINT32_MAX)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "No suitable Vulkan physical device found.");
            return false;
        }

        
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
        if (deviceCount == 0)
        {
            return;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(Instance, &deviceCount, devices.data());

        for (VkPhysicalDevice device : devices)
        {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(device, &properties);

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            if (queueFamilyCount > 0)
            {
                vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
            }

            FQueueCounts queueCounts{};
            for (const VkQueueFamilyProperties &family : queueFamilies)
            {
                if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    queueCounts.Graphics++;
                }
                if (family.queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                    queueCounts.Compute++;
                }
                if (family.queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    queueCounts.Transfer++;
                }
            }
            queueCounts.Present = queueCounts.Graphics;

            const bool isDiscrete = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
            FGxAdapterInfo info(properties.deviceName, properties.vendorID, properties.deviceID, isDiscrete, queueCounts);

            AdapterInfos.emplace_back(info);
        }
    }

    void FVulkanContext::ReleaseInstance()
    {
        if (Instance != VK_NULL_HANDLE)
        {
            if(gDebugMessenger != VK_NULL_HANDLE)
            {
                DestroyDebugUtilsMessengerEXT(Instance, gDebugMessenger, nullptr);
            }

            vkDestroyInstance(Instance, nullptr);
            Instance = VK_NULL_HANDLE;
        }
    }
}
