project(CadmusRHI_Vulkan LANGUAGES CXX)
include(FindVulkan.cmake)

find_package(Vulkan QUIET)
if(NOT Vulkan_FOUND)
	message(FATAL_ERROR "Vulkan SDK not found. Please install the Vulkan SDK and ensure VULKAN_SDK is set.")
endif()

add_library(CadmusRHI_Vulkan SHARED)
add_library(Cadmus::RHI::Vulkan ALIAS CadmusRHI_Vulkan)
set_property(GLOBAL APPEND PROPERTY CADMUS_RHI_BACKEND_TARGETS CadmusRHI_Vulkan)

file(GLOB CADMUS_VULKAN_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB CADMUS_VULKAN_HEADERS CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp)

target_sources(CadmusRHI_Vulkan
	PRIVATE
		${CADMUS_VULKAN_SOURCES}
	PUBLIC
		FILE_SET HEADERS FILES
			${CADMUS_VULKAN_HEADERS})

target_link_libraries(CadmusRHI_Vulkan
	PUBLIC
		Cadmus::RHI
	PRIVATE
		Vulkan::Vulkan
		project_warnings
		project_sanitizers)

target_include_directories(CadmusRHI_Vulkan
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
		$<INSTALL_INTERFACE:include/CadmusRHI/Vulkan>
	PRIVATE
		${Vulkan_INCLUDE_DIRS})

target_compile_features(CadmusRHI_Vulkan PUBLIC cxx_std_20)
target_compile_definitions(CadmusRHI_Vulkan PRIVATE CADMUS_VULKAN_RHI_BUILD_DLL)

include(GNUInstallDirs)
install(TARGETS CadmusRHI_Vulkan EXPORT GlobalTargets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CadmusRHI/Vulkan)
