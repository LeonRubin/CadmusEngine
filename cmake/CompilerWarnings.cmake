# Helper to set a consistent warning level across compilers
function(set_project_warnings tgt)
if(MSVC)
# /W4 warnings, multi-processor compile, treat warnings as errors in CI if you want
target_compile_options(${tgt} INTERFACE
/W4
/permissive-
/Zc:__cplusplus
/EHsc
$<$<CONFIG:Release>:/Oi>
)

target_compile_definitions(${tgt} INTERFACE _CRT_SECURE_NO_WARNINGS)
else()
target_compile_options(${tgt} INTERFACE
-Wall -Wextra -Wpedantic
-Wconversion -Wsign-conversion
-Wold-style-cast -Werror=return-type
-Wshadow -Wnon-virtual-dtor -Woverloaded-virtual
)
endif()
endfunction()