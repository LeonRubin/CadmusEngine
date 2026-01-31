# Console application that links to Engine
add_executable(SampleApplication)

# Sources
target_sources(SampleApplication PRIVATE main.cpp)


# Link to the Engine static lib
target_link_libraries(SampleApplication PRIVATE Cadmus::Engine)

# Pull in all configured RHI backend plugins so their DLLs are built and staged alongside the sample.
if(TARGET Cadmus::RHI::Backends)
    target_link_libraries(SampleApplication PRIVATE Cadmus::RHI::Backends)
endif()

# Ensure SampleApplication rebuilds (and reruns the DLL copy step) when any backend DLL rebuilds.
get_property(CADMUS_RHI_BACKEND_TARGETS GLOBAL PROPERTY CADMUS_RHI_BACKEND_TARGETS)
if(CADMUS_RHI_BACKEND_TARGETS)
    foreach(backend_target IN LISTS CADMUS_RHI_BACKEND_TARGETS)
        add_custom_command(TARGET SampleApplication POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${backend_target}>
            $<TARGET_FILE_DIR:SampleApplication>
        COMMENT "Copying backend ${backend_target} to SampleApplication output")
    endforeach()
endif()


# If you want the sample to see Engine headers via install interface too
target_include_directories(SampleApplication PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


# On Windows, put the exe next to sources (handy for assets)
set_target_properties(SampleApplication PROPERTIES
VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# Copy engine config next to the built executable (Config/...)
add_custom_command(TARGET SampleApplication POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
${CMAKE_SOURCE_DIR}/Engine/Config
$<TARGET_FILE_DIR:SampleApplication>/Config
COMMENT "Copying Engine Config to output directory")


