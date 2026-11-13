#include <Windows.h>
#include <TlHelp32.h>

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace
{
    constexpr wchar_t kAllowedProcessName[] = L"MemoryScannerTest.exe";
    constexpr SIZE_T kChunkSize = 1024 * 1024;

    struct ScanResult
    {
        std::uintptr_t address{};
        std::int32_t value{};
    };

    DWORD FindTestProcessId()
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
            return 0;

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);

        DWORD pid = 0;
        if (Process32FirstW(snapshot, &entry))
        {
            do
            {
                if (_wcsicmp(entry.szExeFile, kAllowedProcessName) == 0)
                {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return pid;
    }

    bool IsReadableProtection(DWORD protect)
    {
        if ((protect & PAGE_GUARD) != 0 || (protect & PAGE_NOACCESS) != 0)
            return false;

        const DWORD base = protect & 0xFF;
        return base == PAGE_READONLY ||
               base == PAGE_READWRITE ||
               base == PAGE_WRITECOPY ||
               base == PAGE_EXECUTE_READ ||
               base == PAGE_EXECUTE_READWRITE ||
               base == PAGE_EXECUTE_WRITECOPY;
    }

    std::vector<ScanResult> ExactScan(HANDLE process, std::int32_t target)
    {
        std::vector<ScanResult> results;
        SYSTEM_INFO info{};
        GetSystemInfo(&info);

        auto address = reinterpret_cast<std::uintptr_t>(info.lpMinimumApplicationAddress);
        const auto maximum = reinterpret_cast<std::uintptr_t>(info.lpMaximumApplicationAddress);

        while (address < maximum)
        {
            MEMORY_BASIC_INFORMATION mbi{};
            if (VirtualQueryEx(process, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) == 0)
                break;

            const auto regionBase = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
            const SIZE_T regionSize = mbi.RegionSize;

            if (mbi.State == MEM_COMMIT && IsReadableProtection(mbi.Protect) &&
                regionSize >= sizeof(std::int32_t))
            {
                std::vector<unsigned char> buffer(kChunkSize + sizeof(std::int32_t));
                SIZE_T offset = 0;

                while (offset < regionSize)
                {
                    const SIZE_T requested = (regionSize - offset > kChunkSize)
                        ? kChunkSize : regionSize - offset;

                    SIZE_T bytesRead = 0;
                    const auto current = regionBase + offset;
                    if (ReadProcessMemory(process, reinterpret_cast<LPCVOID>(current),
                                          buffer.data(), requested, &bytesRead) && bytesRead >= sizeof(target))
                    {
                        for (SIZE_T i = 0; i + sizeof(target) <= bytesRead; ++i)
                        {
                            std::int32_t value{};
                            std::memcpy(&value, buffer.data() + i, sizeof(value));
                            if (value == target)
                            {
                                results.push_back({ current + i, value });
                                if (results.size() >= 100000)
                                    return results;
                            }
                        }
                    }

                    if (requested == 0)
                        break;
                    offset += requested;
                }
            }

            const auto next = regionBase + regionSize;
            if (next <= address)
                break;
            address = next;
        }

        return results;
    }
}

int main()
{
    std::wcout << L"=== MemoryScanner (test-process edition) ===\n";
    std::wcout << L"This build only scans MemoryScannerTest.exe.\n\n";

    const DWORD pid = FindTestProcessId();
    if (pid == 0)
    {
        std::wcout << L"MemoryScannerTest.exe was not found. Start it first.\n";
        std::wcout << L"Press Enter to exit...";
        std::wstring wait;
        std::getline(std::wcin, wait);
        return 1;
    }

    std::wcout << L"Connected to MemoryScannerTest.exe (PID " << pid << L")\n";

    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE |
                                 PROCESS_VM_OPERATION, FALSE, pid);
    if (!process)
    {
        std::wcerr << L"OpenProcess failed. Win32 error: " << GetLastError() << L"\n";
        return 1;
    }

    std::int32_t target{};
    std::wcout << L"Enter the exact 32-bit integer currently shown by the test program: ";
    if (!(std::wcin >> target))
    {
        CloseHandle(process);
        return 1;
    }

    std::wcout << L"Scanning...\n";
    auto results = ExactScan(process, target);
    std::wcout << L"Found " << results.size() << L" matching address(es).\n";

    const std::size_t displayCount = (results.size() < 50) ? results.size() : 50;
    for (std::size_t i = 0; i < displayCount; ++i)
    {
        std::wcout << std::setw(3) << i << L"  0x" << std::hex << results[i].address
                  << std::dec << L"  value=" << results[i].value << "\n";
    }

    if (!results.empty())
    {
        std::size_t selected{};
        std::wcout << L"Select a result index to change (0-" << displayCount - 1
                  << L"), or enter -1 to skip: ";

        long long choice = -1;
        if (std::wcin >> choice && choice >= 0 &&
            static_cast<std::size_t>(choice) < displayCount)
        {
            std::int32_t replacement{};
            std::wcout << L"New 32-bit integer value: ";
            if (std::wcin >> replacement)
            {
                SIZE_T bytesWritten = 0;
                const auto address = reinterpret_cast<LPVOID>(results[static_cast<std::size_t>(choice)].address);
                if (WriteProcessMemory(process, address, &replacement, sizeof(replacement), &bytesWritten) &&
                    bytesWritten == sizeof(replacement))
                {
                    std::wcout << L"Value updated. Check MemoryScannerTest.exe.\n";
                }
                else
                {
                    std::wcout << L"WriteProcessMemory failed. Win32 error: " << GetLastError() << "\n";
                }
            }
        }
    }

    CloseHandle(process);
    std::wcout << L"Done. Press Enter to exit...";
    std::wcin.ignore((std::numeric_limits<std::streamsize>::max)(), L'\n');
    std::wcin.get();
    return 0;
}
