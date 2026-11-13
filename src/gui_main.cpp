#include <Windows.h>
#include <TlHelp32.h>
#include <CommCtrl.h>

#include <cstdint>
#include <cstring>
#include <cwchar>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "Comctl32.lib")

namespace
{
    constexpr wchar_t kAllowedProcessName[] = L"MemoryScannerTest.exe";
    constexpr SIZE_T kChunkSize = 1024 * 1024;
    constexpr std::size_t kMaxResults = 20000;

    constexpr int ID_REFRESH = 1001;
    constexpr int ID_SCAN = 1002;
    constexpr int ID_WRITE = 1003;
    constexpr int ID_VALUE = 1004;
    constexpr int ID_REPLACEMENT = 1005;
    constexpr int ID_RESULTS = 1006;
    constexpr int ID_STATUS = 1007;

    struct ScanResult
    {
        std::uintptr_t address{};
        std::int32_t value{};
    };

    HWND g_window{};
    HWND g_processLabel{};
    HWND g_valueEdit{};
    HWND g_replacementEdit{};
    HWND g_resultsList{};
    HWND g_status{};
    HANDLE g_process{};
    DWORD g_processId{};
    std::vector<ScanResult> g_results;

    std::wstring ToAddress(std::uintptr_t address)
    {
        std::wostringstream stream;
        stream << L"0x" << std::uppercase << std::hex << address;
        return stream.str();
    }

    void SetStatus(const std::wstring& message)
    {
        SetWindowTextW(g_status, message.c_str());
    }

    void CloseTarget()
    {
        if (g_process)
        {
            CloseHandle(g_process);
            g_process = nullptr;
        }
        g_processId = 0;
        g_results.clear();
        if (g_resultsList)
            ListView_DeleteAllItems(g_resultsList);
    }

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

    bool AttachToTestProcess()
    {
        CloseTarget();

        const DWORD pid = FindTestProcessId();
        if (pid == 0)
        {
            SetWindowTextW(g_processLabel, L"Target: MemoryScannerTest.exe not found");
            SetStatus(L"Start MemoryScannerTest.exe first, then click Refresh.");
            return false;
        }

        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ |
                                     PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
                                     FALSE, pid);
        if (!process)
        {
            SetWindowTextW(g_processLabel, L"Target: access denied");
            SetStatus(L"OpenProcess failed. Win32 error: " + std::to_wstring(GetLastError()));
            return false;
        }

        g_process = process;
        g_processId = pid;
        SetWindowTextW(g_processLabel,
            (L"Target: MemoryScannerTest.exe    PID: " + std::to_wstring(pid)).c_str());
        SetStatus(L"Connected. Enter the current test value and click First Scan.");
        return true;
    }

    bool IsReadableProtection(DWORD protect)
    {
        if ((protect & PAGE_GUARD) != 0 || (protect & PAGE_NOACCESS) != 0)
            return false;

        const DWORD base = protect & 0xFF;
        return base == PAGE_READONLY || base == PAGE_READWRITE ||
               base == PAGE_WRITECOPY || base == PAGE_EXECUTE_READ ||
               base == PAGE_EXECUTE_READWRITE || base == PAGE_EXECUTE_WRITECOPY;
    }

    std::vector<ScanResult> ExactScan(HANDLE process, std::int32_t target)
    {
        std::vector<ScanResult> results;
        SYSTEM_INFO info{};
        GetSystemInfo(&info);

        auto address = reinterpret_cast<std::uintptr_t>(info.lpMinimumApplicationAddress);
        const auto maximum = reinterpret_cast<std::uintptr_t>(info.lpMaximumApplicationAddress);

        while (address < maximum && results.size() < kMaxResults)
        {
            MEMORY_BASIC_INFORMATION mbi{};
            if (VirtualQueryEx(process, reinterpret_cast<LPCVOID>(address),
                               &mbi, sizeof(mbi)) == 0)
                break;

            const auto regionBase = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
            const SIZE_T regionSize = mbi.RegionSize;

            if (mbi.State == MEM_COMMIT && IsReadableProtection(mbi.Protect) &&
                regionSize >= sizeof(target))
            {
                std::vector<BYTE> buffer(kChunkSize);
                SIZE_T offset = 0;

                while (offset < regionSize && results.size() < kMaxResults)
                {
                    const SIZE_T requested = (regionSize - offset > kChunkSize)
                        ? kChunkSize : regionSize - offset;
                    SIZE_T bytesRead = 0;
                    const auto current = regionBase + offset;

                    if (ReadProcessMemory(process, reinterpret_cast<LPCVOID>(current),
                                          buffer.data(), requested, &bytesRead))
                    {
                        for (SIZE_T i = 0; i + sizeof(target) <= bytesRead; ++i)
                        {
                            std::int32_t value{};
                            std::memcpy(&value, buffer.data() + i, sizeof(value));
                            if (value == target)
                            {
                                results.push_back({ current + i, value });
                                if (results.size() >= kMaxResults)
                                    break;
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

    void PopulateResults()
    {
        ListView_DeleteAllItems(g_resultsList);
        for (std::size_t i = 0; i < g_results.size(); ++i)
        {
            LVITEMW item{};
            item.mask = LVIF_TEXT;
            item.iItem = static_cast<int>(i);
            const std::wstring index = std::to_wstring(i);
            item.pszText = const_cast<LPWSTR>(index.c_str());
            const int row = ListView_InsertItem(g_resultsList, &item);

            const std::wstring address = ToAddress(g_results[i].address);
            const std::wstring value = std::to_wstring(g_results[i].value);
            ListView_SetItemText(g_resultsList, row, 1, const_cast<LPWSTR>(address.c_str()));
            ListView_SetItemText(g_resultsList, row, 2, const_cast<LPWSTR>(value.c_str()));
        }
    }

    bool ReadIntFromEdit(HWND edit, std::int32_t& value)
    {
        wchar_t text[64]{};
        GetWindowTextW(edit, text, static_cast<int>(_countof(text)));
        wchar_t* end = nullptr;
        const long parsed = std::wcstol(text, &end, 10);
        if (text[0] == L'\0' || end == text || *end != L'\0' ||
            parsed < (std::numeric_limits<std::int32_t>::min)() ||
            parsed > (std::numeric_limits<std::int32_t>::max)())
            return false;

        value = static_cast<std::int32_t>(parsed);
        return true;
    }

    void DoFirstScan()
    {
        if (!g_process || WaitForSingleObject(g_process, 0) == WAIT_OBJECT_0)
        {
            SetStatus(L"Target is not connected or has exited. Click Refresh.");
            return;
        }

        std::int32_t target{};
        if (!ReadIntFromEdit(g_valueEdit, target))
        {
            SetStatus(L"Enter a valid signed 32-bit integer.");
            return;
        }

        SetStatus(L"Scanning memory...");
        EnableWindow(g_window, FALSE);
        g_results = ExactScan(g_process, target);
        PopulateResults();
        EnableWindow(g_window, TRUE);
        SetStatus(L"First Scan complete. Found " + std::to_wstring(g_results.size()) +
                  (g_results.size() == kMaxResults ? L" results (display limit reached)." : L" results."));
    }

    void DoWriteSelected()
    {
        if (!g_process)
        {
            SetStatus(L"Connect to MemoryScannerTest.exe first.");
            return;
        }

        const int selected = ListView_GetNextItem(g_resultsList, -1, LVNI_SELECTED);
        if (selected < 0 || static_cast<std::size_t>(selected) >= g_results.size())
        {
            SetStatus(L"Select an address from the results list first.");
            return;
        }

        std::int32_t replacement{};
        if (!ReadIntFromEdit(g_replacementEdit, replacement))
        {
            SetStatus(L"Enter a valid replacement integer.");
            return;
        }

        SIZE_T written = 0;
        const auto address = reinterpret_cast<LPVOID>(g_results[static_cast<std::size_t>(selected)].address);
        if (WriteProcessMemory(g_process, address, &replacement, sizeof(replacement), &written) &&
            written == sizeof(replacement))
        {
            g_results[static_cast<std::size_t>(selected)].value = replacement;
            ListView_SetItemText(g_resultsList, selected, 2,
                const_cast<LPWSTR>(std::to_wstring(replacement).c_str()));
            SetStatus(L"Value updated in MemoryScannerTest.exe.");
        }
        else
        {
            SetStatus(L"WriteProcessMemory failed. Win32 error: " + std::to_wstring(GetLastError()));
        }
    }

    HWND MakeControl(DWORD exStyle, const wchar_t* className, const wchar_t* text,
                     DWORD style, int x, int y, int width, int height,
                     HWND parent, int id)
    {
        return CreateWindowExW(exStyle, className, text, style,
            x, y, width, height, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
            GetModuleHandleW(nullptr), nullptr);
    }

    void LayoutControls(HWND window)
    {
        RECT rc{};
        GetClientRect(window, &rc);
        const int width = rc.right - rc.left;
        const int height = rc.bottom - rc.top;
        const int margin = 12;

        MoveWindow(g_processLabel, margin, 12, width - margin * 2 - 110, 24, TRUE);
        MoveWindow(GetDlgItem(window, ID_REFRESH), width - 98, 10, 86, 28, TRUE);
        MoveWindow(GetDlgItem(window, ID_VALUE), margin, 52, 190, 26, TRUE);
        MoveWindow(g_valueEdit, 210, 52, 120, 26, TRUE);
        MoveWindow(GetDlgItem(window, ID_SCAN), 340, 52, 110, 28, TRUE);
        MoveWindow(GetDlgItem(window, ID_REPLACEMENT), margin, 88, 190, 26, TRUE);
        MoveWindow(g_replacementEdit, 210, 88, 120, 26, TRUE);
        MoveWindow(GetDlgItem(window, ID_WRITE), 340, 88, 150, 28, TRUE);
        MoveWindow(g_resultsList, margin, 128, width - margin * 2,
                   height - 176 > 80 ? height - 176 : 80, TRUE);
        MoveWindow(g_status, margin, height - 34, width - margin * 2, 22, TRUE);
    }

    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_CREATE:
        {
            g_window = window;
            g_processLabel = MakeControl(0, L"STATIC", L"Target: not connected",
                WS_CHILD | WS_VISIBLE, 12, 12, 500, 24, window, 0);
            MakeControl(0, L"BUTTON", L"Refresh",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, window, ID_REFRESH);
            MakeControl(0, L"STATIC", L"Scan value (Int32):",
                WS_CHILD | WS_VISIBLE, 12, 52, 190, 26, window, ID_VALUE - 10);
            g_valueEdit = MakeControl(WS_EX_CLIENTEDGE, L"EDIT", L"100",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 0, 0, window, ID_VALUE);
            MakeControl(0, L"BUTTON", L"First Scan",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, window, ID_SCAN);
            MakeControl(0, L"STATIC", L"New value:",
                WS_CHILD | WS_VISIBLE, 12, 88, 190, 26, window, ID_REPLACEMENT - 10);
            g_replacementEdit = MakeControl(WS_EX_CLIENTEDGE, L"EDIT", L"999",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 0, 0, window, ID_REPLACEMENT);
            MakeControl(0, L"BUTTON", L"Change Selected",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, window, ID_WRITE);
            g_resultsList = MakeControl(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                12, 128, 700, 300, window, ID_RESULTS);
            ListView_SetExtendedListViewStyle(g_resultsList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

            LVCOLUMNW column{};
            column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            column.pszText = const_cast<LPWSTR>(L"Index");
            column.cx = 70;
            ListView_InsertColumn(g_resultsList, 0, &column);
            column.pszText = const_cast<LPWSTR>(L"Address");
            column.cx = 230;
            column.iSubItem = 1;
            ListView_InsertColumn(g_resultsList, 1, &column);
            column.pszText = const_cast<LPWSTR>(L"Value (Int32)");
            column.cx = 150;
            column.iSubItem = 2;
            ListView_InsertColumn(g_resultsList, 2, &column);

            g_status = MakeControl(0, L"STATIC", L"Start MemoryScannerTest.exe, then click Refresh.",
                WS_CHILD | WS_VISIBLE, 12, 0, 700, 22, window, ID_STATUS);

            AttachToTestProcess();
            return 0;
        }
        case WM_SIZE:
            LayoutControls(window);
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case ID_REFRESH:
                AttachToTestProcess();
                return 0;
            case ID_SCAN:
                DoFirstScan();
                return 0;
            case ID_WRITE:
                DoWriteSelected();
                return 0;
            }
            break;
        case WM_DESTROY:
            CloseTarget();
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    INITCOMMONCONTROLSEX controls{ sizeof(controls), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&controls);

    const wchar_t className[] = L"MemoryScannerMainWindow";
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = className;

    if (!RegisterClassExW(&wc))
        return 1;

    HWND window = CreateWindowExW(0, className, L"MemoryScanner - First GUI",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 820, 620,
        nullptr, nullptr, instance, nullptr);
    if (!window)
        return 1;

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
