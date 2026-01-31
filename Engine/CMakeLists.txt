# Engine static library
add_library(Engine STATIC)
add_library(Cadmus::Engine ALIAS Engine)

# Explicitly set the linker language (needed when no source files exist)
set_target_properties(Engine PROPERTIES LINKER_LANGUAGE CXX)

add_subdirectory(ThirdParty)
# Sources - recursively collect headers and sources
file(GLOB_RECURSE ENGINE_HEADERS CONFIGURE_DEPENDS "include/*.hpp" "include/*.h")
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.c")

# Attach files to target
target_sources(Engine PRIVATE ${ENGINE_SOURCES} PUBLIC FILE_SET HEADERS FILES ${ENGINE_HEADERS})

# Link against SDL (build-time only, not exported)
target_link_libraries(Engine PRIVATE $<BUILD_INTERFACE:SDL3::SDL3>)

# FetchContent toml11 shouldn't be part of the install interface/export set
target_link_libraries(Engine PRIVATE $<BUILD_INTERFACE:toml11::toml11>)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(Engine PRIVATE spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)

# Expose RHI interface headers to Engine consumers
target_link_libraries(Engine PUBLIC Cadmus::RHI)

# Public include dir (consumed by dependents)
target_include_directories(Engine
PUBLIC
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
$<INSTALL_INTERFACE:include>)

# Private include dir (used by Engine's internal sources)
target_include_directories(Engine
PRIVATE
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)


# Compile features/definitions
target_compile_features(Engine PUBLIC cxx_std_20)


# Link helper INTERFACE libs
target_link_libraries(Engine
PUBLIC project_warnings
PUBLIC project_sanitizers)


# Example of a compile definition for consumers
# target_compile_definitions(Engine PUBLIC ENGINE_USE_SOMETHING=1)


# Install headers + library
include(GNUInstallDirs)
install(TARGETS Engine EXPORT GlobalTargets
ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Engine)
