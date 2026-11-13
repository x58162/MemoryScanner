# MemoryScanner

A Windows C++ learning project for scanning and editing values in a dedicated test process that you own.

## Safety scope

The scanner intentionally attaches only to the companion `MemoryScannerTest` process included in this repository. It is not a general-purpose process editor and will refuse other process names. Use it to learn Windows process memory APIs on your own test program.

## Build

Requirements: Windows 10/11, Visual Studio 2022 with **Desktop development with C++**, and CMake.

From a Developer PowerShell:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Executables:
- `build/Release/MemoryScannerTest.exe`
- `build/Release/MemoryScanner.exe`

Run `MemoryScannerTest.exe` first. It displays a changing integer value and waits for input.
Then run `MemoryScanner.exe`, enter the displayed value for an exact scan, and choose a result to update the test value.

## Current milestone

- [x] CMake project scaffold
- [x] Dedicated test target
- [x] Exact 32-bit integer scan
- [x] Result selection and value write-back, restricted to the test target
- [ ] Next Scan / changed-value filtering
- [ ] Win32 graphical interface
- [ ] x86 and x64 build validation

