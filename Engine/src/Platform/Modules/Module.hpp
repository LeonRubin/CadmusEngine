#pragma once

#include "Common/String.hpp"

namespace CDMPlatform
{

    struct FLoadedModuleHandle
    {
        void* ModuleHandle = nullptr;
        CDMString ModuleName;
    };

    FLoadedModuleHandle LoadModule(const CDMString& ModuleName);

    void* LoadModuleFunctionRaw(const FLoadedModuleHandle& Handle, const char* FunctionName);

    template <typename T>
    inline T LoadModuleFunction(const FLoadedModuleHandle& Handle, const char* FunctionName)
    {
        if (!Handle.ModuleHandle || !FunctionName)
        {
            return nullptr;
        }

        return reinterpret_cast<T>(LoadModuleFunctionRaw(Handle, FunctionName));
    }
}

