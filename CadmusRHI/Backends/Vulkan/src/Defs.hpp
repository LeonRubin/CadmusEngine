#if defined(_WIN32)
#    ifdef CADMUS_VULKAN_RHI_BUILD_DLL
#        define CADMUS_VULKAN_RHI_API __declspec(dllexport)
#    else
#        define CADMUS_VULKAN_RHI_API __declspec(dllimport)
#    endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#    define CADMUS_VULKAN_RHI_API __attribute__((visibility("default")))
#else
#    define CADMUS_VULKAN_RHI_API
#endif