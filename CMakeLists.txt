cmake_minimum_required(VERSION 3.16)
project(DeadlockSimulator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Put all output executables in the root build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Compiler warnings and options
if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Global include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Core Simulator Static Library
add_library(deadlock_core STATIC
    src/core/ResourceManager.cpp
    src/prevention/PreventionEngine.cpp
    src/detection/DetectionEngine.cpp
    src/recovery/RecoveryEngine.cpp
    src/bridge/StateSerializer.cpp
    src/bridge/SimulationService.cpp
    src/bridge/HttpServer.cpp
    src/bridge/ApiRouter.cpp
)

target_include_directories(deadlock_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (WIN32)
    target_link_libraries(deadlock_core PUBLIC ws2_32)
endif()

# Interactive Console CLI Executable
add_executable(deadlock_sim
    app/main.cpp
)

target_link_libraries(deadlock_sim PRIVATE
    deadlock_core
)

# HTTP REST API Server Executable
add_executable(deadlock_server
    app/server_main.cpp
)

target_link_libraries(deadlock_server PRIVATE
    deadlock_core
)

# Automated Test Suite (Unit and Integration Tests)
enable_testing()
add_subdirectory(tests)
