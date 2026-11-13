# MemoryScanner

A Windows C++ learning project for scanning and editing values in a dedicated test process that you own.

## Safety scope

The scanner intentionally attaches only to the companion `MemoryScannerTest.exe` process included in this repository. It is not a general-purpose process editor and refuses other process names. Use it to learn Windows process memory APIs on your own test program.

## Build

Requirements: Windows 10/11, Visual Studio 2022 with **Desktop development with C++**, and CMake.

From a Developer PowerShell:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Executables:
- `build/Release/MemoryScanner.exe` — Windows GUI scanner
- `build/Release/MemoryScannerCLI.exe` — console version
- `build/Release/MemoryScannerTest.exe` — dedicated test process

## Run the GUI

1. Start `MemoryScannerTest.exe`. It prints its current integer value (initially 100).
2. Start `MemoryScanner.exe`.
3. Confirm the target label shows the test process PID.
4. Enter the current value in **Scan value (Int32)** and click **First Scan**.
5. Select a result row, enter a replacement integer, and click **Change Selected**.
6. Check the test process output to confirm the value changed. The test process can increment its value by pressing Enter in its console.

## Current milestone

- [x] CMake project scaffold
- [x] Dedicated test target
- [x] Exact 32-bit integer scan
- [x] Result selection and value write-back, restricted to the test target
- [x] First Win32 GUI: target refresh, exact scan, result list, selected-value write
- [ ] Next Scan / changed-value filtering
- [ ] Value types (Byte, 2 Bytes, 4 Bytes, Float, Double)
- [ ] Freeze selected value
- [ ] x86 and x64 build validation

