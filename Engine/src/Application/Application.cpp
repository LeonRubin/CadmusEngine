#include "Engine/Application/Application.h"
#include <SDL3/SDL.h>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

void Init()
{
    SDL_Window *window = nullptr;

    SDL_Surface *screenSurface = nullptr;

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
        
        screenSurface = SDL_GetWindowSurface(window);

        SDL_FillSurfaceRect(screenSurface, NULL, SDL_MapRGB(SDL_GetPixelFormatDetails(screenSurface->format), SDL_GetSurfacePalette(screenSurface), 0xFF, 0xFF, 0xFF));

        SDL_UpdateWindowSurface(window);

        SDL_Event e; bool quit = false; while (!quit) { while (SDL_PollEvent(&e)) { if (e.type == SDL_EVENT_QUIT) { quit = true; } } }
    }
}