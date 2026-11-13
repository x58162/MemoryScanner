cmake_minimum_required(VERSION 3.20)
project(MemoryScanner LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "MemoryScanner currently supports Windows only.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Native Windows GUI application.
add_executable(MemoryScanner WIN32
    src/gui_main.cpp
)
target_compile_definitions(MemoryScanner PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(MemoryScanner PRIVATE /W4 /permissive-)
target_link_libraries(MemoryScanner PRIVATE Comctl32)

# Keep the original console scanner available for comparison and debugging.
add_executable(MemoryScannerCLI
    src/main.cpp
)
target_compile_definitions(MemoryScannerCLI PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(MemoryScannerCLI PRIVATE /W4 /permissive-)

# Dedicated process used to safely test reading and writing.
add_executable(MemoryScannerTest
    src/test_target.cpp
)
target_compile_definitions(MemoryScannerTest PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_compile_options(MemoryScannerTest PRIVATE /W4 /permissive-)
