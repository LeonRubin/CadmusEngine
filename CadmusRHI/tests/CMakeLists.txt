include(FetchContent)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_Declare(
	googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG v1.15.2)
FetchContent_MakeAvailable(googletest)

add_executable(CadmusRHI_BitmappedFreeListTests
	BitmappedFreeList.Tests.cpp)

target_link_libraries(CadmusRHI_BitmappedFreeListTests
	PRIVATE
		Cadmus::RHI
		GTest::gtest_main)

add_test(
	NAME CadmusRHI_BitmappedFreeListTests
	COMMAND CadmusRHI_BitmappedFreeListTests)
