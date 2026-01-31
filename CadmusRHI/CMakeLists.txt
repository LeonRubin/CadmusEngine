# Interface target for shared RHI headers
add_library(CadmusRHI INTERFACE)
add_library(Cadmus::RHI ALIAS CadmusRHI)

set(CADMUS_RHI_INTERFACE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/interface)
set(CADMUS_RHI_DEFAULT_BACKEND "Null" CACHE STRING "Default RHI backend name to load at runtime")

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/interface)
configure_file(${CADMUS_RHI_INTERFACE_DIR}/Defs.hpp.in ${CADMUS_RHI_INTERFACE_DIR}/Defs.hpp @ONLY)

file(GLOB CADMUS_RHI_INTERFACE_HEADERS "${CADMUS_RHI_INTERFACE_DIR}/*.hpp")

target_sources(CadmusRHI INTERFACE FILE_SET HEADERS FILES
	${CADMUS_RHI_INTERFACE_HEADERS})

target_include_directories(CadmusRHI INTERFACE
	$<BUILD_INTERFACE:${CADMUS_RHI_INTERFACE_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/interface>
	$<INSTALL_INTERFACE:include/CadmusRHI>)

target_compile_features(CadmusRHI INTERFACE cxx_std_20)
target_compile_definitions(CadmusRHI INTERFACE CADMUS_RHI_STATIC)

include(GNUInstallDirs)
install(TARGETS CadmusRHI EXPORT GlobalTargets
	FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CadmusRHI)

# Add any backend modules that provide their own CMakeLists.txt
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Backends")
	file(GLOB BACKEND_DIRS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/Backends" "${CMAKE_CURRENT_SOURCE_DIR}/Backends/*")
	foreach(backend ${BACKEND_DIRS})
		if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Backends/${backend}" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Backends/${backend}/CMakeLists.txt")
			add_subdirectory(Backends/${backend})
		endif()
	endforeach()
endif()

# Aggregate backend plugin targets so executables can depend on all configured backends at once.
get_property(CADMUS_RHI_BACKEND_TARGETS GLOBAL PROPERTY CADMUS_RHI_BACKEND_TARGETS)
list(REMOVE_DUPLICATES CADMUS_RHI_BACKEND_TARGETS)

if(CADMUS_RHI_BACKEND_TARGETS)
	add_library(CadmusRHIBackends INTERFACE)
	add_library(Cadmus::RHI::Backends ALIAS CadmusRHIBackends)
	target_link_libraries(CadmusRHIBackends INTERFACE ${CADMUS_RHI_BACKEND_TARGETS})
endif()
