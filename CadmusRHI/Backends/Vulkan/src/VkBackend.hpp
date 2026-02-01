#pragma once

#include "Defs.hpp"

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "Context.hpp"
#include "GxAdapter.hpp"
#include "Swapchain.hpp"
#include "Types.hpp"



namespace rhi::vulkan
{
}

extern "C" CADMUS_VULKAN_RHI_API rhi::IContext* CreateRHIContext();
extern "C" CADMUS_VULKAN_RHI_API void SetRHILoggerFunc(rhi::PFN_LoggerFunc loggerFunc);
