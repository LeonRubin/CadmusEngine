#include "Platform/CdmPaths.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <filesystem>

#define MAX_PATH 512

CDMString CdmPaths::GetCdmRootPath()
{
    wchar_t buffer[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, buffer, MAX_PATH);

    if(length == 0 || length == MAX_PATH)
    {
        return CDMString();
    }

    std::filesystem::path exePath(buffer);
    auto exeDir = exePath.parent_path();
    
    return exeDir.string();
}