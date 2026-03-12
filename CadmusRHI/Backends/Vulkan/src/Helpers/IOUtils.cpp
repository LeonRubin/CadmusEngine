#include "IOUtils.hpp"

#include <filesystem>
#include <fstream>
#include <string>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace io
{
namespace
{
    std::filesystem::path GetExecutableDirectory()
    {
#if defined(_WIN32)
        std::wstring modulePath(MAX_PATH, L'\0');
        DWORD length = GetModuleFileNameW(nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
        if (length == 0)
        {
            return {};
        }

        modulePath.resize(length);
        return std::filesystem::path(modulePath).parent_path();
#else
        return {};
#endif
    }

    std::filesystem::path ResolvePathForRead(const char* path)
    {
        if (path == nullptr)
        {
            return {};
        }

        std::filesystem::path inputPath(path);
        if (inputPath.is_absolute())
        {
            return inputPath;
        }

        if (std::filesystem::exists(inputPath))
        {
            return inputPath;
        }

        std::filesystem::path executableDir = GetExecutableDirectory();
        if (!executableDir.empty())
        {
            std::filesystem::path exeRelativePath = executableDir / inputPath;
            if (std::filesystem::exists(exeRelativePath))
            {
                return exeRelativePath;
            }
        }

        return inputPath;
    }

    std::filesystem::path ResolvePathForWrite(const char* path)
    {
        if (path == nullptr)
        {
            return {};
        }

        std::filesystem::path inputPath(path);
        if (inputPath.is_absolute())
        {
            return inputPath;
        }

        std::filesystem::path executableDir = GetExecutableDirectory();
        if (!executableDir.empty())
        {
            std::filesystem::path exeRelativePath = executableDir / inputPath;
            std::filesystem::path exeParent = exeRelativePath.parent_path();
            if (exeParent.empty() || std::filesystem::exists(exeParent))
            {
                return exeRelativePath;
            }
        }

        return inputPath;
    }

    thread_local std::string GExtStorage;
    thread_local std::string GNameStorage;

    FString DefaultGetExtensionFromPath(const char* path)
    {
        if (path == nullptr)
        {
            GExtStorage.clear();
            return {nullptr, 0};
        }

        std::filesystem::path fsPath(path);
        auto ext = fsPath.extension().string();
        if (!ext.empty() && ext.front() == '.')
        {
            ext.erase(ext.begin());
        }
        GExtStorage = ext;
        return {GExtStorage.c_str(), GExtStorage.size()};
    }

    FString DefaultGetFileName(const char* path)
    {
        if (path == nullptr)
        {
            GNameStorage.clear();
            return {nullptr, 0};
        }

        GNameStorage = std::filesystem::path(path).filename().string();
        return {GNameStorage.c_str(), GNameStorage.size()};
    }

    bool DefaultFileExists(const char* path)
    {
        if (path == nullptr)
        {
            return false;
        }
        return std::filesystem::exists(ResolvePathForRead(path));
    }

    bool DefaultReadFile(const char* path, bool binary, char** outData, std::size_t* outSize)
    {
        if (path == nullptr || outData == nullptr || outSize == nullptr)
        {
            return false;
        }

        std::filesystem::path resolvedPath = ResolvePathForRead(path);
        std::ifstream file(resolvedPath, std::ios::in | (binary ? std::ios::binary : std::ios::openmode{}));
        if (!file)
        {
            return false;
        }

        file.seekg(0, std::ios::end);
        std::streampos endPos = file.tellg();
        if (endPos < 0)
        {
            return false;
        }

        std::size_t size = static_cast<std::size_t>(endPos);
        file.seekg(0, std::ios::beg);

        char* buffer = nullptr;
        std::size_t actualSize = size;
        if (size > 0)
        {
            buffer = new char[size];
            file.read(buffer, static_cast<std::streamsize>(size));
            const std::streamsize bytesRead = file.gcount();
            if (bytesRead < 0)
            {
                delete[] buffer;
                return false;
            }

            if (binary)
            {
                if (static_cast<std::size_t>(bytesRead) != size || !file)
                {
                    delete[] buffer;
                    return false;
                }
            }
            else
            {
                if (file.bad() || (file.fail() && !file.eof()))
                {
                    delete[] buffer;
                    return false;
                }
                actualSize = static_cast<std::size_t>(bytesRead);
            }
        }

        *outData = buffer;
        *outSize = actualSize;
        return true;
    }

    bool DefaultWriteFile(const char* path, bool binary, const void* data, std::size_t size)
    {
        if (path == nullptr || (data == nullptr && size > 0))
        {
            return false;
        }

        std::filesystem::path resolvedPath = ResolvePathForWrite(path);
        std::ofstream file(resolvedPath, std::ios::out | std::ios::trunc | (binary ? std::ios::binary : std::ios::openmode{}));
        if (!file)
        {
            return false;
        }

        if (size > 0)
        {
            file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
            if (!file)
            {
                return false;
            }
        }
        return true;
    }

    void DefaultFreeBuffer(char* buffer)
    {
        delete[] buffer;
    }

    FIOFunctionTable GActiveFunctions{
        DefaultGetExtensionFromPath,
        DefaultGetFileName,
        DefaultFileExists,
        DefaultReadFile,
        DefaultWriteFile,
        DefaultFreeBuffer};
} // namespace io

void SetIOFunctions(const FIOFunctionTable& functions)
{
    if (functions.GetExtensionFromPath)
    {
        GActiveFunctions.GetExtensionFromPath = functions.GetExtensionFromPath;
    }
    if (functions.GetFileName)
    {
        GActiveFunctions.GetFileName = functions.GetFileName;
    }
    if (functions.FileExists)
    {
        GActiveFunctions.FileExists = functions.FileExists;
    }
    if (functions.ReadFile)
    {
        GActiveFunctions.ReadFile = functions.ReadFile;
    }
    if (functions.WriteFile)
    {
        GActiveFunctions.WriteFile = functions.WriteFile;
    }
    if (functions.FreeBuffer)
    {
        GActiveFunctions.FreeBuffer = functions.FreeBuffer;
    }
}

const FIOFunctionTable& GetIOFunctions()
{
    return GActiveFunctions;
}

FString GetExtension(const char* path)
{
    return GActiveFunctions.GetExtensionFromPath(path);
}

FString GetFileName(const char* path)
{
    return GActiveFunctions.GetFileName(path);
}

bool FileExists(const char* path)
{
    return GActiveFunctions.FileExists(path);
}

bool ReadFile(const char* path, bool binary, char** outData, std::size_t* outSize)
{
    return GActiveFunctions.ReadFile(path, binary, outData, outSize);
}

bool WriteFile(const char* path, bool binary, const void* data, std::size_t size)
{
    return GActiveFunctions.WriteFile(path, binary, data, size);
}

void FreeBuffer(char* buffer)
{
    GActiveFunctions.FreeBuffer(buffer);
}
} // namespace rhi::vulkan::io