#include <Windows.h>
#include <atomic>
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
    // This process is the only target accepted by MemoryScanner.
    volatile LONG g_testValue = 100;

    std::cout << "MemoryScannerTest is running. PID: " << GetCurrentProcessId() << "\n";
    std::cout << "Test value address: " << static_cast<const void*>(&g_testValue) << "\n";
    std::cout << "Initial value: " << g_testValue << "\n";
    std::cout << "Press Enter here to increase the value by 1. Type q then Enter to quit.\n";

    std::string line;
    while (std::getline(std::cin, line))
    {
        if (line == "q" || line == "Q")
            break;

        InterlockedIncrement(const_cast<volatile LONG*>(&g_testValue));
        std::cout << "Current value: " << g_testValue << "\n";
    }

    return 0;
}
