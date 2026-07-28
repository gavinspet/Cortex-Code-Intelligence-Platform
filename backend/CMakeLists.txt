cmake_minimum_required(VERSION 3.20)

# Project Definition
project(CortexCodeIntelligencePlatform VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard Configuration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build Configuration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output Directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Source Files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Executable Definition
add_executable(cortex ${SOURCES})

# Include Directories
target_include_directories(cortex PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    /usr/include
    /usr/include/jsoncpp
    /usr/include/spdlog
)

# Find and Link all dependencies: Drogon, trantor, jsoncpp, spdlog, and fmt
target_link_libraries(cortex PRIVATE drogon trantor jsoncpp spdlog fmt)

# Compiler Flags (optional, for better warnings)
if(MSVC)
    target_compile_options(cortex PRIVATE /W4)
else()
    target_compile_options(cortex PRIVATE -Wall -Wextra -Wpedantic)
endif()
