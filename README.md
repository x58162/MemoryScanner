# MemoryScanner

Windows C++ learning project for scanning values in the companion test process that you own.

## Open directly in Visual Studio 2022 (CMake is not required)

1. Clone or pull this GitHub repository.
2. Open **MemoryScanner.sln** in Visual Studio 2022.
3. Choose **x64** (recommended) and **Release** in the toolbar.
4. Right-click the solution in Solution Explorer and select **Build Solution**.
5. Executables are written to `bin\x64\Release\`.

The solution contains three projects:
- **MemoryScanner** — Win32 GUI scanner.
- **MemoryScannerTest** — companion console process containing the sample integer.
- **MemoryScannerCLI** — command-line version.

For a test: start `MemoryScannerTest.exe` first, then start `MemoryScanner.exe`. The GUI currently offers an exact signed Int32 First Scan, a result list, and selected-result write-back. It intentionally only attaches to the companion test process named `MemoryScannerTest.exe`.

## Architecture

The Visual Studio solution includes **Win32** and **x64** configurations. Build the scanner and test target with the same architecture; x64 is recommended on 64-bit Windows.

## Optional CMake

`CMakeLists.txt` is retained for developers who prefer CMake. It is optional when using `MemoryScanner.sln`.

## Safety scope

This project is intentionally restricted to the included `MemoryScannerTest.exe`, so it can be used to learn Windows process-memory APIs against a test program you own.

## Current implementation

- [x] Native Visual Studio solution and project files
- [x] Win32 and x64 configurations
- [x] Dedicated companion test process
- [x] Exact signed 32-bit integer scan
- [x] Result list and selected-result write-back
- [ ] Next Scan / changed-value filtering
- [ ] Additional value types (Byte, Int16, Float, Double)
- [ ] Freeze selected test value
- [ ] Windows build and runtime validation
