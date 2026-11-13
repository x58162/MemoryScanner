cmake_minimum_required(VERSION 3.20)
project(MemoryScanner LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "MemoryScanner currently supports Windows only.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(MemoryScanner
    src/main.cpp
)
target_compile_definitions(MemoryScanner PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(MemoryScanner PRIVATE /W4 /permissive-)

add_executable(MemoryScannerTest
    src/test_target.cpp
)
target_compile_definitions(MemoryScannerTest PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(MemoryScannerTest PRIVATE /W4 /permissive-)
