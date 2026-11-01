cmake_minimum_required(VERSION 3.16)
project(UnifiedEngineeringWorkbench VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release build if unspecified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Compiler options & static runtime for standalone execution
if(MSVC)
    add_compile_options(/W4 /permissive-)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# -----------------------------------------------------------------------------
# Core Library: Pure C++ numerical & data logic (no UI, no HTTP, no console I/O)
# -----------------------------------------------------------------------------
set(CORE_SOURCES
    src/core/thermal_fea.cpp
    src/core/backprop.cpp
    src/core/parallel_batch.cpp
    src/core/cad_ansys_parser.cpp
    src/core/embedded_device.cpp
)

add_library(workbench_core STATIC ${CORE_SOURCES})
target_include_directories(workbench_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# -----------------------------------------------------------------------------
# Unit Tests Runner
# -----------------------------------------------------------------------------
set(TEST_SOURCES
    tests/test_main.cpp
    tests/test_thermal_fea.cpp
    tests/test_backprop.cpp
    tests/test_parallel_batch.cpp
    tests/test_cad_ansys_parser.cpp
    tests/test_embedded_device.cpp
)

add_executable(workbench_tests ${TEST_SOURCES})
target_link_libraries(workbench_tests PRIVATE workbench_core)
if(WIN32 AND NOT MSVC)
    target_link_libraries(workbench_tests PRIVATE ws2_32 wsock32)
endif()
enable_testing()
add_test(NAME AllTests COMMAND workbench_tests)

# -----------------------------------------------------------------------------
# Localhost Server (CLI binary in bin/)
# -----------------------------------------------------------------------------
add_executable(workbench_server
    src/server/server_main.cpp
    src/server/workbench_server.cpp
)
target_link_libraries(workbench_server PRIVATE workbench_core)
if(WIN32 AND NOT MSVC)
    target_link_libraries(workbench_server PRIVATE ws2_32 wsock32)
endif()
set_target_properties(workbench_server PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

# -----------------------------------------------------------------------------
# One-Click Launch.exe (Root executable, GUI subsystem: no terminal popup)
# -----------------------------------------------------------------------------
if(WIN32)
    add_executable(Launch WIN32
        src/launcher/launcher_main.cpp
        src/server/workbench_server.cpp
    )
    target_link_libraries(Launch PRIVATE workbench_core ws2_32 wsock32)
    set_target_properties(Launch PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# -----------------------------------------------------------------------------
# CLI Verification Tools
# -----------------------------------------------------------------------------
add_executable(generate_fea_results tools/generate_fea_results.cpp)
target_link_libraries(generate_fea_results PRIVATE workbench_core)

add_executable(generate_backprop_results tools/generate_backprop_results.cpp)
target_link_libraries(generate_backprop_results PRIVATE workbench_core)

add_executable(generate_multicore_benchmark tools/generate_multicore_benchmark.cpp)
target_link_libraries(generate_multicore_benchmark PRIVATE workbench_core)

add_executable(test_file_size_limits tools/test_file_size_limits.cpp)
target_link_libraries(test_file_size_limits PRIVATE workbench_core)
if(WIN32 AND NOT MSVC)
    target_link_libraries(test_file_size_limits PRIVATE psapi)
endif()
