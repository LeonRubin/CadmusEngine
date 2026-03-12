// Basic file/path helpers with overridable function table.
#pragma once

#include "../Defs.hpp"
#include <cstddef>
#include "String.hpp"

namespace io
{

	using PFN_GetExtensionFromPath = FString (*)(const char* path);
	using PFN_GetFileName = FString (*)(const char* path);
	using PFN_FileExists = bool (*)(const char* path);
	using PFN_ReadFile = bool (*)(const char* path, bool binary, char** outData, std::size_t* outSize);
	using PFN_WriteFile = bool (*)(const char* path, bool binary, const void* data, std::size_t size);
	using PFN_FreeBuffer = void (*)(char* buffer);

	struct FIOFunctionTable
	{
		PFN_GetExtensionFromPath GetExtensionFromPath = nullptr;
		PFN_GetFileName GetFileName = nullptr;
		PFN_FileExists FileExists = nullptr;
		PFN_ReadFile ReadFile = nullptr;
		PFN_WriteFile WriteFile = nullptr;
		PFN_FreeBuffer FreeBuffer = nullptr;
	};

	// Allows parent applications to replace default IO helpers.
	CADMUS_VULKAN_RHI_API void SetIOFunctions(const FIOFunctionTable& functions);
	CADMUS_VULKAN_RHI_API const FIOFunctionTable& GetIOFunctions();

	// Convenience wrappers that dispatch through the current function table.
	CADMUS_VULKAN_RHI_API FString GetExtension(const char* path);
	CADMUS_VULKAN_RHI_API FString GetFileName(const char* path);
	CADMUS_VULKAN_RHI_API bool FileExists(const char* path);
	CADMUS_VULKAN_RHI_API bool ReadFile(const char* path, bool binary, char** outData, std::size_t* outSize);
	CADMUS_VULKAN_RHI_API bool WriteFile(const char* path, bool binary, const void* data, std::size_t size);
	CADMUS_VULKAN_RHI_API void FreeBuffer(char* buffer);
} // namespace io

