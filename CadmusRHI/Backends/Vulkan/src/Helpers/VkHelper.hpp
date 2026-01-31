#pragma once

#include <cassert>
#include <cstdio>

#include <Context.hpp>

#define VK_CHECK_RESULT(fnc)                                                               \
    do                                                                                     \
    {                                                                                      \
        VkResult res = (fnc);                                                              \
        if (res != VK_SUCCESS)                                                             \
        {                                                                                  \
            if (gLoggerFunc)                                                               \
            {                                                                              \
                char buf[256];                                                             \
                std::snprintf(buf, sizeof(buf), "Vulkan error at %s:%d in %s returned %d", \
                              __FILE__, __LINE__, #fnc, static_cast<int>(res));            \
                gLoggerFunc(rhi::RHI_LOGLEVEL_ERROR, buf);                                 \
            }                                                                              \
            assert(res == VK_SUCCESS);                                                     \
        }                                                                                  \
    } while (0)

void LogRHI(rhi::ERHILogLevel level, const char *message)
{
    if (rhi::vulkan::gLoggerFunc)
    {
        rhi::vulkan::gLoggerFunc(level, message);
    }
};
