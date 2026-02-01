#include "Engine/Application/Application.h"
#include <SDL3/SDL.h>
#include <Context.hpp>
#include <toml.hpp>
#include "Common/String.hpp"
#include "Platform/CdmPaths.hpp"
#include <Platform/Modules/Module.hpp>
#include <SDL3/SDL_vulkan.h>
#include "spdlog/spdlog.h"

#ifdef _WIN32
#include <windows.h>
#endif

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// To be moved to Application class later
bool gInitialized = false;
SDL_Window *window = nullptr;
SDL_Surface *screenSurface = nullptr;
rhi::IContext *gRHIContext = nullptr;

void Init()
{
    spdlog::info("Initializing Cadmus Engine...");
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%H:%M:%S] [%^%l%$] %v");

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        SDL_Log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    }
    else
    {

        window = SDL_CreateWindow("Cadmus Engine", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
        if (window == nullptr)
        {
            SDL_Log("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        }

        gInitialized = true;

        screenSurface = SDL_GetWindowSurface(window);

        SDL_FillSurfaceRect(screenSurface, NULL, SDL_MapRGB(SDL_GetPixelFormatDetails(screenSurface->format), SDL_GetSurfacePalette(screenSurface), 0xFF, 0xFF, 0xFF));

        CDMString rootPath = CdmPaths::GetCdmRootPath();

        std::filesystem::path configPath = std::filesystem::path(rootPath) / "Config" / "Engine.toml";
        auto configData = toml::try_parse(configPath, toml::spec::v(1, 1, 0));

        CDMString rendererBackend = "Vulkan";

        if (configData.is_ok())
        {
            auto parsedData = configData.as_ok();
            if (parsedData.contains("Renderer"))
            {
                rendererBackend = toml::find_or<std::string>(parsedData, "RendererBackend", "Vulkan");
            }
        }

        CDMPlatform::FLoadedModuleHandle rendererModule = CDMPlatform::LoadModule("cadmusRHI_" + rendererBackend + ".dll");
        if (rendererModule.ModuleHandle)
        {
            auto setRHILoggerFunc = CDMPlatform::LoadModuleFunction<rhi::PFN_SetLoggerFunc>(rendererModule, "SetRHILoggerFunc");
            if (setRHILoggerFunc)
            {
                setRHILoggerFunc([](rhi::ERHILogLevel level, const char *message)
                                 {
                    switch(level)
                    {
                        case rhi::RHI_LOGLEVEL_DEBUG:
                            spdlog::debug("[RHI] {}", message);
                            break;
                        case rhi::RHI_LOGLEVEL_INFO:
                            spdlog::info("[RHI] {}", message);
                            break;
                        case rhi::RHI_LOGLEVEL_WARNING:
                            spdlog::warn("[RHI] {}", message);
                            break;
                        case rhi::RHI_LOGLEVEL_ERROR:
                            spdlog::error("[RHI] {}", message);
                            break;
                    } });
            }

            auto createContextFunc = CDMPlatform::LoadModuleFunction<rhi::PFN_CreateRHIContext>(rendererModule, "CreateRHIContext");
            if (createContextFunc)
            {
                gRHIContext = createContextFunc();

                rhi::FContextInitParams initParams;
                Uint32 count = 0;
                initParams.RequiredInstanceExtensions = SDL_Vulkan_GetInstanceExtensions(&count);
                initParams.NumRequiredInstanceExtensions = static_cast<int>(count);
#ifdef _WIN32
                auto props = SDL_GetWindowProperties(window);
                initParams.WindowHandle = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr); 
#endif

                gRHIContext->Initialize(initParams);
            }
        }
    }
}

bool IterateEngineLoop()
{
    SDL_Event e;
    bool quit = false;

    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_EVENT_QUIT)
        {
            quit = true;
        }
    }

    return quit;
}

void DeInit()
{
    // deinit
    if (gInitialized)
    {
        delete gRHIContext;
        gRHIContext = nullptr;
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
}