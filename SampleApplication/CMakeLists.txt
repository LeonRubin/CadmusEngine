# Console application that links to Engine
add_executable(SampleApplication)

# Sources
target_sources(SampleApplication PRIVATE main.cpp)


# Link to the Engine static lib
target_link_libraries(SampleApplication PRIVATE Cadmus::Engine)


# If you want the sample to see Engine headers via install interface too
target_include_directories(SampleApplication PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


# On Windows, put the exe next to sources (handy for assets)
set_target_properties(SampleApplication PROPERTIES
VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})