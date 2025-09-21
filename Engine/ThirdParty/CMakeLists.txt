set(SDL_TEST OFF)
set(SDL_SHARED OFF CACHE BOOL "Build shared SDL library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build static SDL library" FORCE)

add_subdirectory(SDL)

add_library(thirdparty INTERFACE)
target_link_libraries(thirdparty INTERFACE SDL3::SDL3)