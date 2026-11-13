#pragma once

#include <Windows.h>
#include <string>
#include <vector>

struct ProcessInfo
{
    DWORD pid;
    std::wstring name;
};

class ProcessManager
{
public:
    static std::vector<ProcessInfo> GetProcesses();
};
