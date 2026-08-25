cmake_minimum_required(VERSION 3.20)

project(cpu_task_scheduler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Put the built program in build/bin so it is easy to find.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(scheduler cli/main.cpp)

# Turn on helpful compiler warnings.
if(MSVC)
    target_compile_options(scheduler PRIVATE /W4)
else()
    target_compile_options(scheduler PRIVATE -Wall -Wextra)
endif()
