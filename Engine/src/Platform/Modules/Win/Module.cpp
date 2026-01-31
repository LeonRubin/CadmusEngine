#include "Platform/Modules/Module.hpp"
#include <Windows.h>
#include <tchar.h>
#include <libloaderapi.h>
#include <assert.h>

using namespace CDMPlatform;

FLoadedModuleHandle CDMPlatform::LoadModule(const CDMString &ModuleName)
{
    std::wstring wideModuleName(ModuleName.begin(), ModuleName.end());
    HMODULE ModuleHandle = LoadLibraryA(ModuleName.c_str());
    if (ModuleHandle == NULL)
    {   
        return {NULL, ""};
    }
    
    return {ModuleHandle, ModuleName};
}

void *CDMPlatform::LoadModuleFunctionRaw(const FLoadedModuleHandle &Handle, const char *FunctionName)
{
    assert(Handle.ModuleHandle != nullptr);
    
    FARPROC procAddress = GetProcAddress(static_cast<HMODULE>(Handle.ModuleHandle), FunctionName);
    
    return reinterpret_cast<void*>(procAddress);

}