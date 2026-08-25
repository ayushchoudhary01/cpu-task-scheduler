cmake_minimum_required(VERSION 3.20)

project(cpu_task_scheduler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Put built programs in build/bin so they are easy to find.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Helpful compiler warnings, applied to every target below.
if(MSVC)
    set(WARNING_FLAGS /W4)
else()
    set(WARNING_FLAGS -Wall -Wextra)
endif()

# The scheduling logic. Both the CLI and the tests use this.
add_library(scheduler_core
    core/Timeline.cpp
    core/Metrics.cpp
    core/Engine.cpp
    core/policies/FcfsPolicy.cpp
    core/policies/SjfPolicy.cpp
    core/policies/SrtfPolicy.cpp
    core/policies/RoundRobinPolicy.cpp
    core/policies/PriorityPolicy.cpp
    core/PolicyRegistry.cpp
)
target_include_directories(scheduler_core PUBLIC core)
target_compile_options(scheduler_core PRIVATE ${WARNING_FLAGS})

# Command line handling. A library rather than part of main.cpp, so the tests
# can exercise argument and workload parsing directly.
add_library(scheduler_cli
    cli/WorkloadParser.cpp
    cli/CommandLine.cpp
    cli/TextReport.cpp
    cli/JsonReport.cpp
)
target_include_directories(scheduler_cli PUBLIC cli)
target_link_libraries(scheduler_cli PUBLIC scheduler_core)
target_compile_options(scheduler_cli PRIVATE ${WARNING_FLAGS})

# The command line program.
add_executable(scheduler cli/main.cpp)
target_link_libraries(scheduler PRIVATE scheduler_cli)
target_compile_options(scheduler PRIVATE ${WARNING_FLAGS})

# The tests.
enable_testing()
add_executable(scheduler_tests
    tests/main.cpp
    tests/CoreTests.cpp
    tests/AlgorithmTests.cpp
    tests/PriorityTests.cpp
    tests/CliTests.cpp
)
target_link_libraries(scheduler_tests PRIVATE scheduler_cli)
target_include_directories(scheduler_tests PRIVATE tests)
target_compile_options(scheduler_tests PRIVATE ${WARNING_FLAGS})
add_test(NAME unit_tests COMMAND scheduler_tests)
