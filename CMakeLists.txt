cmake_minimum_required(VERSION 3.28)

project(CadmusEngineROOT VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#Options
option(BUILD_EXAMPLES "Build samples (SampleApplication)" ON)
option(ENABLE_WARNINGS "Enable project warning presets" ON)
option(ENABLE_SANITIZERS "Enable sanitizers in Debug on non-MSVC" ON)
option(ENABLE_UNITY "Enable unity/jumbo build for faster compiles" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)

add_library(project_warnings INTERFACE )
if(ENABLE_WARNINGS)
set_project_warnings(project_warnings)
endif()

add_library(project_sanitizers INTERFACE)
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
target_compile_options(project_sanitizers INTERFACE -fsanitize=address,undefined)
target_link_options(project_sanitizers INTERFACE -fsanitize=address,undefined)
endif()

# Install the helper interface libraries so they can be exported
install(TARGETS project_warnings project_sanitizers EXPORT GlobalTargets)

if(ENABLE_UNITY)
set(CMAKE_UNITY_BUILD ON)
endif()

add_subdirectory(Engine)

add_subdirectory(SampleApplication)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(EXPORT GlobalTargets
NAMESPACE Cadmus::
FILE GlobalTargets.cmake
DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Cadmus)

configure_package_config_file(
${CMAKE_CURRENT_SOURCE_DIR}/cmake/CadmusConfig.cmake.in
${CMAKE_CURRENT_BINARY_DIR}/CadmusConfig.cmake
INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Cadmus)
