#include "VkContext.hpp"

#include <vector>
#include "Helpers/VkHelper.hpp"
#include <algorithm>
#include <map>

#ifdef _WIN32
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#include "Pipeline/VkPipelineBuilder.hpp"
#include "ImmediateCommandsBufferManager.hpp"
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

        for (const char *requested : validationLayers)
        {
            const bool found = std::any_of(availableLayers.begin(), availableLayers.end(),
                                           [requested](const VkLayerProperties &props)
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

    bool checkDeviceExtSupport(VkPhysicalDevice device, const std::vector<const char *> &requiredExtensions)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        for (const char *requiredExt : requiredExtensions)
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
                std::snprintf(buf, sizeof(buf), "Required Vulkan Device Extension not available: %s", requiredExt);
                LogRHI(rhi::RHI_LOGLEVEL_ERROR, buf);
                return false;
            }
        }

        return true;
    }

    uint32_t FVulkanContext::SelectPhysicalDevice(const std::vector<VkPhysicalDevice> &physicalDevices, int desiredIdx)
    {
        VkPhysicalDeviceProperties properties{};
        VkPhysicalDeviceFeatures features{};

        std::multimap<int, int> candidates;

        for (int i = 0; i < static_cast<int>(physicalDevices.size()); ++i)
        {
            vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);
            vkGetPhysicalDeviceFeatures(physicalDevices[i], &features);

            int score = 0;

            bool bCompatible = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
            bCompatible &= features.geometryShader == VK_TRUE;

            std::vector<rhi::vulkan::FVkQueueFamilyInfo> queueFamilyInfos;
            EnumerateQueueFamilies(physicalDevices[i], queueFamilyInfos, Surface);

            uint32_t graphicsQueueFamilyIndex = GetQueueFamilyWithFeatures(queueFamilyInfos, EQueueFeatures::GRAPHICS) != UINT32_MAX;
            bCompatible &= graphicsQueueFamilyIndex != UINT32_MAX;

            VkBool32 presentSupported;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[i], graphicsQueueFamilyIndex, Surface, &presentSupported);
            bCompatible &= presentSupported == VK_TRUE;
            bCompatible &= checkDeviceExtSupport(physicalDevices[i], {VK_KHR_SWAPCHAIN_EXTENSION_NAME});

            FSwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(physicalDevices[i], Surface);
            bCompatible &= !swapchainSupport.Formats.empty() && !swapchainSupport.PresentModes.empty();

            score += properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 10 : 0;
            score += properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? 5 : 0;

            candidates.insert({score, i});

            if (bCompatible && i == desiredIdx && desiredIdx != -1)
            {
                return i;
            }
        }

        if (candidates.begin()->first > 0)
        {
            return candidates.rbegin()->second;
        }
        else
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

        if (AdapterInfos.empty())
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "No Vulkan physical devices found.");
            return false;
        }

#ifdef WIN32
        // Create Surface for Windows
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
        surfaceCreateInfo.hwnd = static_cast<HWND>(Params.WindowHandle);

        VK_CHECK_RESULT(vkCreateWin32SurfaceKHR(Instance, &surfaceCreateInfo, nullptr, &Surface));
#endif

        uint32_t deviceCount = AdapterInfos.size();
        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(Instance, &deviceCount, physicalDevices.data());

        auto physicalDeviceID = SelectPhysicalDevice(physicalDevices, Params.DesiredDeviceIdx);
        if (physicalDeviceID == UINT32_MAX)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "No suitable Vulkan physical device found.");
            return false;
        }

        std::vector<FVkQueueFamilyInfo> QueueFamilyInfos;
        EnumerateQueueFamilies(physicalDevices[physicalDeviceID], QueueFamilyInfos, Surface);
        ActiveQueueFamilyInfos = QueueFamilyInfos;
        PhysicalDevice = physicalDevices[physicalDeviceID];

        std::vector<FVkQueueFamilyInfo> RequiredQueueFamilies;

        // Request Graphics, Compute Dedicated and Transfer Dedicated + anyone which supports present
        // if no dedicated infos, we still get the compute or compute&transfer queue family in any case
        for (int i = 0; i < static_cast<int>(QueueFamilyInfos.size()); ++i)
        {
            bool bRequired = false;

            bRequired |= (QueueFamilyInfos[i].SupportsFeatures(EQueueFeatures::GRAPHICS));
            bRequired |= (QueueFamilyInfos[i].SupportsFeatures(EQueueFeatures::COMPUTE) && !QueueFamilyInfos[i].SupportsFeatures(EQueueFeatures::GRAPHICS));
            bRequired |= (QueueFamilyInfos[i].SupportsFeatures(EQueueFeatures::TRANSFER) && !QueueFamilyInfos[i].SupportsFeatures(EQueueFeatures::GRAPHICS) && !QueueFamilyInfos[i].SupportsFeatures(EQueueFeatures::COMPUTE));
            bRequired |= (QueueFamilyInfos[i].bSupportsPresent);

            if (bRequired)
                RequiredQueueFamilies.push_back(QueueFamilyInfos[i]);
        }

        FVkAdapterCreateInfo adapterCreateInfo{};
        adapterCreateInfo.PhysicalDevice = PhysicalDevice;
        adapterCreateInfo.QueueFamilyInfos = RequiredQueueFamilies.data();
        adapterCreateInfo.NumQueueFamilies = RequiredQueueFamilies.size();
        adapterCreateInfo.NumQueuesPerFamily = 3; // God Loves 3

        SelectedAdapter = new VkGraphicsAdapter(AdapterInfos[physicalDeviceID], adapterCreateInfo);

        Swapchain = new VkSwapchain(SelectedAdapter->GetDevice(), PhysicalDevice, Surface);

        ImmediateCmdBufferManager = new ImmediateCommandsBufferManager(this);
        return true;
    }

    rhi::IPipelineBuilder* FVulkanContext::GetPipelineBuilder()
    {
        return new VkPipelineBuilder(SelectedAdapter->GetDevice());
    }

    rhi::ICommandBuffersPool* FVulkanContext::GetCommandBuffersPool(EQueueFeatures RequiredFeatures)
    {
        (void)RequiredFeatures;
        return nullptr;
    }

    rhi::ICommandBuffer* FVulkanContext::AcquireImmediateCommandBuffer(EQueueFeatures RequiredFeatures)
    {
        assert(ImmediateCmdBufferManager != nullptr);
        return ImmediateCmdBufferManager->Acquire(RequiredFeatures);
    }

    VkDevice FVulkanContext::GetDevice() const
    {
        return SelectedAdapter != nullptr ? SelectedAdapter->GetDevice() : VK_NULL_HANDLE;
    }

    bool FVulkanContext::AcquireImmediateCommandPool(EQueueFeatures RequiredFeatures, VkCommandPool& OutPool, uint32_t& OutQueueFamilyIndex, VkQueue& OutQueue, EQueueFeatures& OutQueueFeatures)
    {
        OutPool = VK_NULL_HANDLE;
        OutQueueFamilyIndex = 0;
        OutQueue = VK_NULL_HANDLE;
        OutQueueFeatures = EQueueFeatures::None;

        for (const FVkImmediatePoolEntry& poolEntry : ImmediatePools)
        {
            if ((poolEntry.QueueFeatures & RequiredFeatures) == RequiredFeatures)
            {
                OutPool = poolEntry.Pool;
                OutQueueFamilyIndex = poolEntry.QueueFamilyIndex;
                OutQueueFeatures = poolEntry.QueueFeatures;
                vkGetDeviceQueue(GetDevice(), poolEntry.QueueFamilyIndex, 0, &OutQueue);
                return OutPool != VK_NULL_HANDLE && OutQueue != VK_NULL_HANDLE;
            }
        }

        if (!FindQueueFamilyForFeatures(RequiredFeatures, OutQueueFamilyIndex, OutQueueFeatures))
        {
            return false;
        }

        VkCommandPoolCreateInfo poolCreateInfo{};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolCreateInfo.queueFamilyIndex = OutQueueFamilyIndex;

        VK_CHECK_RESULT(vkCreateCommandPool(GetDevice(), &poolCreateInfo, nullptr, &OutPool));
        vkGetDeviceQueue(GetDevice(), OutQueueFamilyIndex, 0, &OutQueue);

        if (OutPool == VK_NULL_HANDLE || OutQueue == VK_NULL_HANDLE)
        {
            if (OutPool != VK_NULL_HANDLE)
            {
                vkDestroyCommandPool(GetDevice(), OutPool, nullptr);
            }
            OutPool = VK_NULL_HANDLE;
            return false;
        }

        ImmediatePools.push_back(FVkImmediatePoolEntry{
            .Pool = OutPool,
            .QueueFamilyIndex = OutQueueFamilyIndex,
            .QueueFeatures = OutQueueFeatures});

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
            if (gDebugMessenger != VK_NULL_HANDLE)
            {
                DestroyDebugUtilsMessengerEXT(Instance, gDebugMessenger, nullptr);
            }

            vkDestroySurfaceKHR(Instance, Surface, nullptr);

            if (SelectedAdapter)
            {
                VkDevice device = SelectedAdapter->GetDevice();
                for (const FVkImmediatePoolEntry& poolEntry : ImmediatePools)
                {
                    if (poolEntry.Pool != VK_NULL_HANDLE)
                    {
                        vkDestroyCommandPool(device, poolEntry.Pool, nullptr);
                    }
                }
                ImmediatePools.clear();

                delete SelectedAdapter;
                SelectedAdapter = nullptr;
            }

            vkDestroyInstance(Instance, nullptr);
            Instance = VK_NULL_HANDLE;
        }
    }

    bool FVulkanContext::FindQueueFamilyForFeatures(EQueueFeatures RequiredFeatures, uint32_t& OutQueueFamilyIndex, EQueueFeatures& OutQueueFeatures) const
    {
        for (const FVkQueueFamilyInfo& queueFamilyInfo : ActiveQueueFamilyInfos)
        {
            if (queueFamilyInfo.SupportsFeatures(RequiredFeatures))
            {
                OutQueueFamilyIndex = queueFamilyInfo.FamilyIndex;
                OutQueueFeatures = queueFamilyInfo.Type;
                return true;
            }
        }

        return false;
    }
}
