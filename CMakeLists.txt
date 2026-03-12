cmake_minimum_required(VERSION 3.28)

project(CadmusEngineROOT VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)

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

include(FetchContent)
FetchContent_Declare(
  toml11
  GIT_REPOSITORY https://github.com/ToruNiina/toml11.git
  GIT_TAG        v4.4.0
)
FetchContent_MakeAvailable(toml11)

add_subdirectory(CadmusRHI)

add_subdirectory(Engine)

add_subdirectory(SampleApplication)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

function(cadmus_collect_targets_recursive out_var dir)
  get_property(local_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
  set(all_targets ${local_targets})

  get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
  foreach(subdir IN LISTS subdirs)
    cadmus_collect_targets_recursive(sub_targets "${subdir}")
    list(APPEND all_targets ${sub_targets})
  endforeach()

  set(${out_var} "${all_targets}" PARENT_SCOPE)
endfunction()

function(cadmus_target_links_engine out_var target_name)
  set(links_engine OFF)
  foreach(link_property IN ITEMS LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
    get_target_property(link_items ${target_name} ${link_property})
    if(NOT link_items)
      continue()
    endif()

    foreach(link_item IN LISTS link_items)
      if(link_item STREQUAL "Engine" OR link_item STREQUAL "Cadmus::Engine")
        set(links_engine ON)
        break()
      endif()
    endforeach()

    if(links_engine)
      break()
    endif()
  endforeach()

  set(${out_var} ${links_engine} PARENT_SCOPE)
endfunction()

set(cadmus_install_engine_content OFF)
set(cadmus_engine_content_dir "${CMAKE_SOURCE_DIR}/Engine/Content")
cadmus_collect_targets_recursive(cadmus_all_targets "${CMAKE_SOURCE_DIR}")
list(REMOVE_DUPLICATES cadmus_all_targets)

foreach(target_name IN LISTS cadmus_all_targets)
  get_target_property(target_imported ${target_name} IMPORTED)
  if(target_imported)
    continue()
  endif()

  get_target_property(target_type ${target_name} TYPE)
  if(NOT target_type STREQUAL "EXECUTABLE"
     AND NOT target_type STREQUAL "SHARED_LIBRARY"
     AND NOT target_type STREQUAL "MODULE_LIBRARY")
    continue()
  endif()

  cadmus_target_links_engine(links_engine ${target_name})
  if(links_engine)
    if(EXISTS "${cadmus_engine_content_dir}")
      string(MAKE_C_IDENTIFIER "${target_name}" cadmus_copy_suffix)
      set(cadmus_copy_target "cadmus_copy_engine_content_${cadmus_copy_suffix}")
      add_custom_target(${cadmus_copy_target}
          COMMAND ${CMAKE_COMMAND} -E make_directory
                  "$<TARGET_FILE_DIR:${target_name}>"
          COMMAND ${CMAKE_COMMAND} -E copy_directory
                  "${cadmus_engine_content_dir}"
                  "$<TARGET_FILE_DIR:${target_name}>/Content"
          COMMENT "Copying Engine Content to ${target_name} output directory")
      add_dependencies(${target_name} ${cadmus_copy_target})
    endif()

    set(cadmus_install_engine_content ON)
  endif()
endforeach()

if(cadmus_install_engine_content AND EXISTS "${cadmus_engine_content_dir}")
  install(DIRECTORY "${cadmus_engine_content_dir}/"
      DESTINATION "${CMAKE_INSTALL_BINDIR}/Content")
endif()

install(EXPORT GlobalTargets
NAMESPACE Cadmus::
FILE GlobalTargets.cmake
DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Cadmus)

configure_package_config_file(
${CMAKE_CURRENT_SOURCE_DIR}/cmake/CadmusConfig.cmake.in
${CMAKE_CURRENT_BINARY_DIR}/CadmusConfig.cmake
INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Cadmus)
