cmake_minimum_required(VERSION 3.20)

project(fxmc
    VERSION 0.1.0
    DESCRIPTION "Monte Carlo simulation of exchange rates"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(FXMC_BUILD_TESTS "Build unit tests" ON)
option(FXMC_BUILD_BENCHMARKS "Build benchmarks" ON)

# ---- Library ----------------------------------------------------------------
add_library(fxmc
    src/models.cpp
    src/simulator.cpp
    src/statistics.cpp
    src/pricing.cpp)
add_library(fxmc::fxmc ALIAS fxmc)
target_include_directories(fxmc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(fxmc PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(fxmc PRIVATE /W4 /permissive-)
else()
    target_compile_options(fxmc PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

# ---- Demo executable --------------------------------------------------------
add_executable(fxmc_demo app/main.cpp)
target_link_libraries(fxmc_demo PRIVATE fxmc::fxmc)

# ---- Third-party dependencies ----------------------------------------------
include(FetchContent)

if(FXMC_BUILD_TESTS)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW TRUE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()

if(FXMC_BUILD_BENCHMARKS)
    FetchContent_Declare(googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.1
        GIT_SHALLOW TRUE)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googlebenchmark)

    add_subdirectory(bench)
endif()
