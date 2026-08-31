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

# Source Files (explicitly listed, excluding deprecated implementations)
set(SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/app/ApplicationFactory.cpp
    src/config/Config.cpp
    src/config/ConfigurationValidator.cpp
    src/config/FileConfiguration.cpp
    src/logging/Logger.cpp
    src/api/health/HealthResponse.cpp
    src/api/health/HealthService.cpp
    src/api/health/HealthController.cpp
    src/api/repositories/RepositoryService.cpp
    src/api/repositories/RepositoryController.cpp
    src/api/jobs/JobService.cpp
    src/api/jobs/JobController.cpp
    src/analysis/AnalysisController.cpp
    src/database/Database.cpp
    src/infrastructure/MySQLJobRepository.cpp
    src/infrastructure/MySQLAnalysisRepository.cpp
    src/worker/JobWorker.cpp
    src/worker/JobWorkerPool.cpp
    src/worker/RedisStreamJobQueue.cpp
    src/core/di/ServiceContainer.cpp
    src/github/GitHubClient.cpp
    src/github/GitHubMetadataService.cpp
    src/technology/TechnologyService.cpp
    src/health/RepositoryHealthService.cpp
    src/insight/RepositoryInsightService.cpp
)

# Executable Definition
add_executable(cortex ${SOURCES})

# Include Directories
target_include_directories(cortex PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    /usr/include
    /usr/include/jsoncpp
    /usr/include/spdlog
    /usr/include/cppconn
)

# Find and Link all dependencies: Drogon, trantor, jsoncpp, spdlog, fmt, MySQL Connector/C++, OpenSSL, ZLIB, UUID, and c-ares
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
target_link_libraries(cortex PRIVATE drogon trantor jsoncpp spdlog fmt mysqlcppconn OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB uuid cares)
# Compiler Flags (optional, for better warnings)
if(MSVC)
    target_compile_options(cortex PRIVATE /W4)
else()
    target_compile_options(cortex PRIVATE -Wall -Wextra -Wpedantic)
endif()
