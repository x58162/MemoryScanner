
#include <Windows.h>
#include <string>
#include <vector>

#include "ProcessManager.h"

#define ID_PROCESS_LIST 1001
#define ID_REFRESH      1002
#define ID_SELECT       1003

HWND hList;
HWND hStatus;

std::vector<ProcessInfo> processList;

void RefreshProcesses()
{
    processList = ProcessManager::GetProcesses();

    SendMessageW(hList, LB_RESETCONTENT, 0, 0);

    for (const auto& process : processList)
    {
        std::wstring item =
            process.name +
            L"    PID: " +
            std::to_wstring(process.pid);

        SendMessageW(
            hList,
            LB_ADDSTRING,
            0,
            reinterpret_cast<LPARAM>(item.c_str())
        );
    }

    std::wstring status =
        L"Processes found: " +
        std::to_wstring(processList.size());

    SetWindowTextW(hStatus, status.c_str());
}

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        hList = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"LISTBOX",
            nullptr,
            WS_CHILD | WS_VISIBLE |
            WS_VSCROLL | LBS_NOTIFY |
            LBS_NOINTEGRALHEIGHT,
            20, 20, 540, 330,
            hwnd,
            (HMENU)ID_PROCESS_LIST,
            nullptr,
            nullptr
        );

        CreateWindowW(
            L"BUTTON",
            L"Refresh",
            WS_CHILD | WS_VISIBLE,
            20, 370, 120, 35,
            hwnd,
            (HMENU)ID_REFRESH,
            nullptr,
            nullptr
        );

        CreateWindowW(
            L"BUTTON",
            L"Select Process",
            WS_CHILD | WS_VISIBLE,
            155, 370, 140, 35,
            hwnd,
            (HMENU)ID_SELECT,
            nullptr,
            nullptr
        );

        hStatus = CreateWindowW(
            L"STATIC",
            L"Ready",
            WS_CHILD | WS_VISIBLE,
            20, 420, 400, 25,
            hwnd,
            nullptr,
            nullptr,
            nullptr
        );

        RefreshProcesses();
        return 0;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case ID_REFRESH:
            RefreshProcesses();
            return 0;

        case ID_SELECT:
        {
            int index = (int)SendMessageW(
                hList,
                LB_GETCURSEL,
                0,
                0
            );

            if (index == LB_ERR ||
                index >= (int)processList.size())
            {
                MessageBoxW(
                    hwnd,
                    L"Please select a process.",
                    L"Memory Scanner",
                    MB_OK
                );
                return 0;
            }

            const auto& process = processList[index];

            std::wstring message =
                L"Selected: " + process.name +
                L"\nPID: " +
                std::to_wstring(process.pid);

            MessageBoxW(
                hwnd,
                message.c_str(),
                L"Process Selected",
                MB_OK
            );

            return 0;
        }
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"MemoryScannerWindow";

    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc))
        return 1;

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Memory Scanner - Process Manager",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        610,
        510,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd)
        return 1;

    ShowWindow(hwnd, nCmdShow);

    MSG msg{};

    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
