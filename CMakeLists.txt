cmake_minimum_required(VERSION 3.16)
project(op2utilitydll LANGUAGES C CXX)

# RC = Windows resource compiler (windres on MinGW), for the version-info resource.
if (WIN32)
    enable_language(RC)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# All of OP2Utility's sources (it has no CMake of its own; we vendor-build it).
file(GLOB_RECURSE OP2UTIL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/OP2Utility/src/*.cpp")

# The DLL = OP2Utility (C++ internals) + the C-ABI wrapper (the only exports).
add_library(op2utility SHARED
    ${OP2UTIL_SOURCES}
    capi/op2utility_c.cpp
    $<$<BOOL:${WIN32}>:capi/version.rc>
)

target_include_directories(op2utility PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/OP2Utility/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/OP2Utility/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/capi"
)

# Only the C wrapper is dllexport'ed; the C++ internals stay hidden.
target_compile_definitions(op2utility PRIVATE OP2UTILITYC_BUILD)

# Clean name: OP2Utility.dll (+ import lib libOP2Utility.dll.a on MinGW).
# Note: case-sensitive output name (capital "OP2U").
set_target_properties(op2utility PROPERTIES PREFIX "" OUTPUT_NAME "OP2Utility")

# Make the DLL self-contained: bake the C++ runtime in so consumers don't need a
# matching libstdc++/libgcc/winpthread. Safe because only a C ABI crosses out.
if (MINGW)
    target_link_options(op2utility PRIVATE
        -static-libgcc -static-libstdc++ -static)
endif()

# Smoke test: a pure-C program that links the DLL via its import lib and the C
# header — proves the boundary resolves and the exception->error-code path works.
add_executable(op2utility_smoketest capi/smoketest.c)
target_include_directories(op2utility_smoketest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/capi")
target_link_libraries(op2utility_smoketest PRIVATE op2utility)
