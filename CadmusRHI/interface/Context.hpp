#pragma once

#include "Defs.hpp"
#include "Types.hpp"

namespace rhi
{
    enum ERHILogLevel
    {
        RHI_LOGLEVEL_DEBUG,
        RHI_LOGLEVEL_INFO,
        RHI_LOGLEVEL_WARNING,
        RHI_LOGLEVEL_ERROR
    };

    struct CADMUS_RHI_API FContextInitParams
    {
        int DesiredDeviceIdx = 0;
        int NumRequiredInstanceExtensions = 0;
        char const * const * RequiredInstanceExtensions = nullptr;
        const bool EnableValidationLayers = true;
        const bool EnableNativeLogging = true;
    };

    class CADMUS_RHI_API IContext
    {
    public:
        virtual ~IContext() = default;

        virtual bool Initialize(const FContextInitParams& Params) = 0;
    };

    typedef IContext* (*PFN_CreateRHIContext)();
    typedef void (*PFN_LoggerFunc)(ERHILogLevel, const char*);
    typedef void (*PFN_SetLoggerFunc)(PFN_LoggerFunc);
}