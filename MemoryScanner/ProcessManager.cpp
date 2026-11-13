
#include "ProcessManager.h"
#include <TlHelp32.h>

std::vector<ProcessInfo> ProcessManager::GetProcesses()
{
    std::vector<ProcessInfo> processes;

    HANDLE snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS,
        0
    );

    if (snapshot == INVALID_HANDLE_VALUE)
        return processes;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            ProcessInfo info;
            info.pid = entry.th32ProcessID;
            info.name = entry.szExeFile;

            processes.push_back(info);

        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);

    return processes;
}
