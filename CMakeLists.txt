cmake_minimum_required(VERSION 3.20)
project(AdaptiveDS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optimization & warning flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

# Library include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Core AdaptiveDS Library
set(ADAPTIVE_DS_SOURCES
    src/adaptive_ds/Types.cpp
    src/adaptive_ds/WorkloadMonitor.cpp
    src/adaptive_ds/AdaptationEngine.cpp
    src/adaptive_ds/AdaptationHistory.cpp
)

add_library(adaptive_ds STATIC ${ADAPTIVE_DS_SOURCES})
target_include_directories(adaptive_ds PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Automated Test Suite
set(TEST_SOURCES
    tests/test_main.cpp
    tests/test_dynamic_array.cpp
    tests/test_doubly_linked_list.cpp
    tests/test_hash_table.cpp
    tests/test_avl_tree.cpp
    tests/test_workload_monitor.cpp
    tests/test_adaptation_engine.cpp
    tests/test_safe_migration.cpp
    tests/test_adaptive_ds.cpp
)

add_executable(adaptive_ds_tests ${TEST_SOURCES})
target_link_libraries(adaptive_ds_tests PRIVATE adaptive_ds)
target_include_directories(adaptive_ds_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Benchmarks
add_executable(bench_adaptive_ds benchmarks/bench_adaptive_vs_stl.cpp)
target_link_libraries(bench_adaptive_ds PRIVATE adaptive_ds)

# Interactive Demo Application
add_executable(demo_adaptive_ds demo/demo_interactive.cpp)
target_link_libraries(demo_adaptive_ds PRIVATE adaptive_ds)

# Enable CTest
enable_testing()
add_test(NAME AllUnitTests COMMAND adaptive_ds_tests)
