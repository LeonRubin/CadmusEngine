#include "VkBackend.hpp"
#include "VkContext.hpp"

#include <utility>

namespace rhi::vulkan
{
PFN_LoggerFunc gLoggerFunc = nullptr;
}

extern "C" CADMUS_VULKAN_RHI_API rhi::IContext *CreateRHIContext()
{
    return new rhi::vulkan::FVulkanContext();
}

extern "C" CADMUS_VULKAN_RHI_API void SetRHILoggerFunc(rhi::PFN_LoggerFunc loggerFunc)
{
    rhi::vulkan::gLoggerFunc = loggerFunc;
}
