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
        (void)messageType;
        (void)pUserData;

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

        uint32_t deviceCount = static_cast<uint32_t>(AdapterInfos.size());
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

        const VkDevice device = SelectedAdapter->GetDevice();
        if (device == VK_NULL_HANDLE)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Initialize failed: selected Vulkan adapter returned a null device.");
            return false;
        }

        PresentQueue = VK_NULL_HANDLE;
        PresentQueueFamilyIndex = 0;
        for (const FVkQueueFamilyInfo &queueFamilyInfo : ActiveQueueFamilyInfos)
        {
            if (queueFamilyInfo.bSupportsPresent)
            {
                PresentQueueFamilyIndex = queueFamilyInfo.FamilyIndex;
                vkGetDeviceQueue(device, PresentQueueFamilyIndex, 0, &PresentQueue);
                break;
            }
        }

        if (PresentQueue == VK_NULL_HANDLE)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Initialize failed: unable to acquire a present-capable queue.");
            return false;
        }

        Swapchain = new VkSwapchain(device, PhysicalDevice, Surface);

        SwapchainTextureHandles.clear();
        SwapchainTextureHandles.reserve(Swapchain->GetDesc().BufferCount);
        for (uint32_t imageIndex = 0; imageIndex < Swapchain->GetDesc().BufferCount; ++imageIndex)
        {
            VkVulkanTexture* swapchainTexture = Swapchain->GetSwapchainTexture(imageIndex);
            if (swapchainTexture == nullptr)
            {
                LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Initialize failed: swapchain texture is null.");
                return false;
            }

            if (swapchainTexture->GetDevice() == VK_NULL_HANDLE ||
                swapchainTexture->GetImage() == VK_NULL_HANDLE ||
                swapchainTexture->GetImageView() == VK_NULL_HANDLE)
            {
                LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Initialize failed: swapchain texture contains null Vulkan handles.");
                return false;
            }

            THandle<VkVulkanTexture> swapchainTextureHandle{};
            TextureHandles.AllocateHandle(
                swapchainTextureHandle,
                GetDevice(),
                swapchainTexture->GetImage(),
                swapchainTexture->GetImageView(),
                swapchainTexture->GetCreateInfo(),
                false,
                false);

            SwapchainTextureHandles.push_back(swapchainTextureHandle);
        }

        ImmediateCmdBufferManager = new ImmediateCommandsBufferManager(this);

        VkFenceCreateInfo acquireFenceCreateInfo{};
        acquireFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        const VkResult acquireFenceResult = vkCreateFence(device, &acquireFenceCreateInfo, nullptr, &SwapchainImageAcquireFence);
        if (acquireFenceResult != VK_SUCCESS || SwapchainImageAcquireFence == VK_NULL_HANDLE)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Initialize failed: unable to create swapchain image-acquire fence.");
            return false;
        }

        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        SwapchainRenderFinishedSemaphores.assign(Swapchain->GetDesc().BufferCount, VK_NULL_HANDLE);
        for (uint32_t imageIndex = 0; imageIndex < Swapchain->GetDesc().BufferCount; ++imageIndex)
        {
            VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
            const VkResult rfResult = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphore);
            if (rfResult != VK_SUCCESS || renderFinishedSemaphore == VK_NULL_HANDLE)
            {
                LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Initialize failed: unable to create swapchain render-finished semaphore.");
                return false;
            }

            SwapchainRenderFinishedSemaphores[imageIndex] = renderFinishedSemaphore;
        }

        bSwapchainImageAcquired = false;
        AcquiredSwapchainImageIndex = INVALID_U32;

        return true;
    }

    rhi::IPipelineBuilder *FVulkanContext::GetPipelineBuilder()
    {
        return new VkPipelineBuilder(SelectedAdapter->GetDevice());
    }

    rhi::ICommandBuffersPool *FVulkanContext::GetCommandBuffersPool(EQueueFeatures RequiredFeatures)
    {
        (void)RequiredFeatures;
        return nullptr;
    }

    rhi::ICommandBuffer *FVulkanContext::AcquireImmediateCommandBuffer(EQueueFeatures RequiredFeatures)
    {
        assert(ImmediateCmdBufferManager != nullptr);
        return ImmediateCmdBufferManager->Acquire(RequiredFeatures);
    }

    VkDevice FVulkanContext::GetDevice() const
    {
        return SelectedAdapter != nullptr ? SelectedAdapter->GetDevice() : VK_NULL_HANDLE;
    }

    bool FVulkanContext::AcquireImmediateCommandPool(EQueueFeatures RequiredFeatures, VkCommandPool &OutPool, uint32_t &OutQueueFamilyIndex, VkQueue &OutQueue, EQueueFeatures &OutQueueFeatures)
    {
        OutPool = VK_NULL_HANDLE;
        OutQueueFamilyIndex = 0;
        OutQueue = VK_NULL_HANDLE;
        OutQueueFeatures = EQueueFeatures::None;

        for (const FVkImmediatePoolEntry &poolEntry : ImmediatePools)
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

    THandle<ITexture> FVulkanContext::CreateTexture(const FTextureCreateInfo &CreateInfo)
    {
        THandle<ITexture> invalidHandle{INVALID_U32, INVALID_U32};
        const VkDevice device = GetDevice();

        if (device == VK_NULL_HANDLE || PhysicalDevice == VK_NULL_HANDLE)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: Vulkan context is not initialized.");
            return invalidHandle;
        }

        if (CreateInfo.Format == EColorFormat::Unknown)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: unknown texture format.");
            return invalidHandle;
        }

        if (CreateInfo.Extent.Width == 0 || CreateInfo.Extent.Height == 0 || CreateInfo.Extent.Depth == 0)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: texture extent must be greater than zero.");
            return invalidHandle;
        }

        if (CreateInfo.MipLevels == 0)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: mip level count must be greater than zero.");
            return invalidHandle;
        }

        if (CreateInfo.ArrayLayers == 0)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: array layer count must be greater than zero.");
            return invalidHandle;
        }

        if (CreateInfo.Usage == ETextureUsageFlags::None)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: texture usage flags cannot be empty.");
            return invalidHandle;
        }

        switch (CreateInfo.Type)
        {
        case ETextureType::Texture2D:
            if (CreateInfo.Extent.Depth != 1)
            {
                LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: 2D textures must have depth equal to 1.");
                return invalidHandle;
            }
            break;
        case ETextureType::Texture3D:
            if (CreateInfo.ArrayLayers != 1)
            {
                LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: 3D textures cannot use array layers.");
                return invalidHandle;
            }

            if (CreateInfo.SampleCount != 1)
            {
                LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: 3D textures do not support multisampling.");
                return invalidHandle;
            }
            break;
        default:
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: unsupported texture type.");
            return invalidHandle;
        }

        const VkFormat vkFormat = ConvertColorFormat(CreateInfo.Format);
        if (vkFormat == VK_FORMAT_UNDEFINED)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: texture format is not supported by Vulkan conversion.");
            return invalidHandle;
        }

        VkSampleCountFlagBits vkSampleCount = VK_SAMPLE_COUNT_1_BIT;
        if (CreateInfo.SampleCount > 64)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: sample count exceeds maximum supported value.");
            return invalidHandle;
        }

        vkSampleCount = ConvertSampleCount(CreateInfo.SampleCount);

        const VkImageType vkImageType = ConvertImageType(CreateInfo.Type);
        const VkImageUsageFlags vkUsageFlags = ConvertTextureUsageFlags(CreateInfo.Usage);

        VkImageFormatProperties formatProperties{};
        const VkResult formatResult = vkGetPhysicalDeviceImageFormatProperties(
            PhysicalDevice,
            vkFormat,
            vkImageType,
            VK_IMAGE_TILING_OPTIMAL,
            vkUsageFlags,
            0,
            &formatProperties);

        if (formatResult != VK_SUCCESS)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: requested texture configuration is not supported by the selected Vulkan device.");
            return invalidHandle;
        }

        const uint32_t requestedDepth = CreateInfo.Type == ETextureType::Texture2D ? 1u : CreateInfo.Extent.Depth;
        if (CreateInfo.Extent.Width > formatProperties.maxExtent.width ||
            CreateInfo.Extent.Height > formatProperties.maxExtent.height ||
            requestedDepth > formatProperties.maxExtent.depth)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: requested texture extent exceeds device limits.");
            return invalidHandle;
        }

        if (CreateInfo.MipLevels > formatProperties.maxMipLevels)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: requested mip count exceeds device limits.");
            return invalidHandle;
        }

        if (CreateInfo.Type != ETextureType::Texture3D && CreateInfo.ArrayLayers > formatProperties.maxArrayLayers)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: requested array layer count exceeds device limits.");
            return invalidHandle;
        }

        if ((formatProperties.sampleCounts & vkSampleCount) == 0)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "CreateTexture failed: requested sample count is not supported for this texture configuration.");
            return invalidHandle;
        }

        THandle<VkVulkanTexture> textureHandle{};
        TextureHandles.AllocateHandle(textureHandle, device, PhysicalDevice, CreateInfo);

        return THandle<ITexture>{textureHandle.Index, textureHandle.Generation};
    }

    VkVulkanTexture *FVulkanContext::GetTexture(const THandle<ITexture> &TextureHandle)
    {
        return TextureHandles.Get(RebindHandle<VkVulkanTexture>(TextureHandle));
    }

    THandle<ITexture> FVulkanContext::GetCurrentSwapchainTexture()
    {
        THandle<ITexture> invalidHandle{INVALID_U32, INVALID_U32};

        if (Swapchain == nullptr || SwapchainImageAcquireFence == VK_NULL_HANDLE)
        {
            return invalidHandle;
        }

        if (!bSwapchainImageAcquired)
        {
            VK_CHECK_RESULT(vkResetFences(GetDevice(), 1, &SwapchainImageAcquireFence));
            uint32_t imageIndex = 0;
            const VkResult acquireResult = vkAcquireNextImageKHR(
                GetDevice(),
                Swapchain->GetHandle(),
                UINT64_MAX,
                VK_NULL_HANDLE,
                SwapchainImageAcquireFence,
                &imageIndex);

            if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
            {
                LogRHI(rhi::RHI_LOGLEVEL_ERROR, "GetCurrentSwapchainTexture failed: vkAcquireNextImageKHR was unsuccessful.");
                return invalidHandle;
            }

            VK_CHECK_RESULT(vkWaitForFences(GetDevice(), 1, &SwapchainImageAcquireFence, VK_TRUE, UINT64_MAX));

            Swapchain->SetCurrentImageIndex(imageIndex);
            AcquiredSwapchainImageIndex = imageIndex;
            bSwapchainImageAcquired = true;
        }

        const uint32_t currentImageIndex = Swapchain->GetCurrentImageIndex();
        if (currentImageIndex >= SwapchainTextureHandles.size())
        {
            return invalidHandle;
        }

        const THandle<VkVulkanTexture>& textureHandle = SwapchainTextureHandles[currentImageIndex];
        return THandle<ITexture>{textureHandle.Index, textureHandle.Generation};
    }

    void FVulkanContext::SubmitCmdBufferImmediate(const ICommandBuffer *pCommandBuffers, size_t CommandBufferCount, bool bSyncWithPreviousSubmits)
    {
        assert(ImmediateCmdBufferManager != nullptr);
        assert(pCommandBuffers != nullptr);

        VkSemaphore externalSignalSemaphore = VK_NULL_HANDLE;
        if (bSwapchainImageAcquired)
        {
            if (AcquiredSwapchainImageIndex < SwapchainRenderFinishedSemaphores.size())
            {
                externalSignalSemaphore = SwapchainRenderFinishedSemaphores[AcquiredSwapchainImageIndex];
            }
        }

        const bool submitted = ImmediateCmdBufferManager->Submit(
            pCommandBuffers,
            CommandBufferCount,
            bSyncWithPreviousSubmits,
            VK_NULL_HANDLE,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            externalSignalSemaphore);
        (void)submitted;
    }

    bool FVulkanContext::Present(bool bSyncWithPreviousSubmits)
    {
        if (Swapchain == nullptr || PresentQueue == VK_NULL_HANDLE || !bSwapchainImageAcquired)
        {
            return false;
        }

        VkSemaphore waitSemaphore = VK_NULL_HANDLE;
        uint32_t waitSemaphoreCount = 0;
        if (bSyncWithPreviousSubmits)
        {
            if (AcquiredSwapchainImageIndex < SwapchainRenderFinishedSemaphores.size())
            {
                waitSemaphore = SwapchainRenderFinishedSemaphores[AcquiredSwapchainImageIndex];
            }

            if (waitSemaphore == VK_NULL_HANDLE)
            {
                LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Present failed: no render-finished semaphore available for the acquired swapchain image.");
                return false;
            }

            waitSemaphoreCount = 1;
        }

        const uint32_t imageIndex = Swapchain->GetCurrentImageIndex();
        VkSwapchainKHR swapchainHandle = Swapchain->GetHandle();
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = waitSemaphoreCount;
        presentInfo.pWaitSemaphores = waitSemaphoreCount > 0 ? &waitSemaphore : nullptr;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchainHandle;
        presentInfo.pImageIndices = &imageIndex;

        const VkResult presentResult = vkQueuePresentKHR(PresentQueue, &presentInfo);
        if (presentResult != VK_SUCCESS && presentResult != VK_SUBOPTIMAL_KHR)
        {
            LogRHI(rhi::RHI_LOGLEVEL_ERROR, "Present failed: vkQueuePresentKHR returned an error.");
            return false;
        }

        bSwapchainImageAcquired = false;
        AcquiredSwapchainImageIndex = INVALID_U32;
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

                delete ImmediateCmdBufferManager;
                ImmediateCmdBufferManager = nullptr;

                if (SwapchainImageAcquireFence != VK_NULL_HANDLE)
                {
                    vkDestroyFence(device, SwapchainImageAcquireFence, nullptr);
                    SwapchainImageAcquireFence = VK_NULL_HANDLE;
                }

                for (VkSemaphore semaphore : SwapchainRenderFinishedSemaphores)
                {
                    if (semaphore != VK_NULL_HANDLE)
                    {
                        vkDestroySemaphore(device, semaphore, nullptr);
                    }
                }
                SwapchainRenderFinishedSemaphores.clear();

                TextureHandles.Clear();
                SwapchainTextureHandles.clear();
                delete Swapchain;
                Swapchain = nullptr;
                for (const FVkImmediatePoolEntry &poolEntry : ImmediatePools)
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

            PresentQueue = VK_NULL_HANDLE;
            PresentQueueFamilyIndex = 0;
            bSwapchainImageAcquired = false;
            AcquiredSwapchainImageIndex = INVALID_U32;

            vkDestroyInstance(Instance, nullptr);
            Instance = VK_NULL_HANDLE;
        }
    }

    bool FVulkanContext::FindQueueFamilyForFeatures(EQueueFeatures RequiredFeatures, uint32_t &OutQueueFamilyIndex, EQueueFeatures &OutQueueFeatures) const
    {
        for (const FVkQueueFamilyInfo &queueFamilyInfo : ActiveQueueFamilyInfos)
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
