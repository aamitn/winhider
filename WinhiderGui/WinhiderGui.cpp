#include <windows.h>
#include <algorithm>
#include <commctrl.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <map>
#include "framework.h"
#include "WinhiderGui.h"
#include <Psapi.h>    // For GetModuleFileNameEx
#include <ShellAPI.h> // For ExtractIconEx

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "comctl32.lib")

#define MAX_LOADSTRING 100

// Control IDs
#define IDC_PROCESS_LIST 1001
#define IDC_BTN_HIDE 1002
#define IDC_BTN_UNHIDE 1003
#define IDC_BTN_HIDETASK 1004
#define IDC_BTN_UNHIDETASK 1005
#define IDC_BTN_REFRESH 1006
#define IDC_RADIO_64BIT 1007
#define IDC_RADIO_32BIT 1008
#define IDT_PROCESS_REFRESH_TIMER 2001 // Timer ID
#define IDC_CHK_AUTOREFRESH 1009
#define IDC_BTN_KILL 1010


// Global Variables:
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
bool show64bit = true; // Default to 64-bit
bool autoRefreshEnabled = true;
int g_processRefreshIntervalMs = 1000; // Timer interval in milliseconds - 1 second
HIMAGELIST g_hImageList = NULL;
struct ProcessState {
    bool screenShareHidden;
    bool taskbarHidden;
};
std::map<DWORD, ProcessState> hiddenProcesses;

// Forward declarations
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void                PopulateProcessList(HWND hList, bool show64bit);
void                RunWinHiderCommand(const std::wstring& args, HWND hWnd, bool use64bit);

bool IsProcess64Bit(DWORD pid) {
    BOOL isWow64 = FALSE;
    BOOL success = FALSE;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess) {
#if defined(_WIN64)
        // On 64-bit OS, check if process is running under WOW64 (i.e., 32-bit)
        success = IsWow64Process(hProcess, &isWow64);
        CloseHandle(hProcess);
        return success ? !isWow64 : false; // 64-bit if not WOW64
#else
        // On 32-bit OS, all processes are 32-bit
        CloseHandle(hProcess);
        return false;
#endif
    }
    return false;
}

bool IsProcess32Bit(DWORD pid) {
    BOOL isWow64 = FALSE;
    BOOL success = FALSE;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess) {
#if defined(_WIN64)
        success = IsWow64Process(hProcess, &isWow64);
        CloseHandle(hProcess);
        return success ? isWow64 : false; // 32-bit if WOW64
#else
        // On 32-bit OS, all processes are 32-bit
        CloseHandle(hProcess);
        return true;
#endif
    }
    return false;
}



// Add this function to extract process icons
HICON GetProcessIcon(const WCHAR* processPath) {
    HICON hIcon = NULL;

    // Try to extract icon from the executable
    ExtractIconEx(processPath, 0, NULL, &hIcon, 1);

    if (!hIcon) {
        // If failed, load default application icon
        hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }

    return hIcon;
}

// Add this function to get process path
std::wstring GetProcessPath(DWORD processID) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
    if (hProcess) {
        WCHAR path[MAX_PATH];
        if (GetModuleFileNameEx(hProcess, NULL, path, MAX_PATH)) {
            CloseHandle(hProcess);
            return path;
        }
        CloseHandle(hProcess);
    }
    return L"";
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR    lpCmdLine,
                      _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icex);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINHIDERGUI, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINHIDERGUI));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

//  FUNCTION: MyRegisterClass() PURPOSE: Registers the window class.
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINHIDERGUI));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINHIDERGUI);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex);
}

//   FUNCTION: InitInstance(HINSTANCE, int) PURPOSE: Saves instance handle and creates main window
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

	// Default Window Size and Position
    HWND hWnd = CreateWindowW(szWindowClass, szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0,
        650, 700,  // Even larger for better usability
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
        return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

void PopulateProcessList(HWND hList, bool show64bit)
{
    // --- Save selection and scroll position ---
    int selectedIndex = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    WCHAR selectedPid[16] = L"";
    if (selectedIndex != -1) {
        ListView_GetItemText(hList, selectedIndex, 1, selectedPid, 16);
    }
    int oldTopIndex = (int)SendMessage(hList, LVM_GETTOPINDEX, 0, 0);

    // --- Get actual row height ---
    int rowHeight = 16; // fallback
    if (ListView_GetItemCount(hList) > 0) {
        RECT rc;
        rc.left = LVIR_BOUNDS;
        if (SendMessage(hList, LVM_GETITEMRECT, 0, (LPARAM)&rc)) {
            rowHeight = rc.bottom - rc.top;
        }
    }

    // --- Disable redraw and batch updates ---
    SendMessage(hList, WM_SETREDRAW, FALSE, 0);

    // --- Repopulate the list ---
    ListView_DeleteAllItems(hList);

    ImageList_RemoveAll(g_hImageList);  // Clear existing icons

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        SendMessage(hList, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(hList, NULL, TRUE);
        return;
    }
    PROCESSENTRY32W pe = { sizeof(pe) };
    int idx = 0, newSelectedIndex = -1;
    if (Process32FirstW(hSnap, &pe)) {
        do {
            bool is64 = IsProcess64Bit(pe.th32ProcessID);
            bool is32 = IsProcess32Bit(pe.th32ProcessID);
            if ((show64bit && is64) || (!show64bit && is32)) {
                // Get process icon
                std::wstring processPath = GetProcessPath(pe.th32ProcessID);
                HICON hIcon = GetProcessIcon(processPath.c_str());
                int imageIndex = ImageList_AddIcon(g_hImageList, hIcon);
                DestroyIcon(hIcon);

                // Add item with icon
                LVITEMW item = { 0 };
                item.mask = LVIF_TEXT | LVIF_IMAGE;
                item.iItem = idx;
                item.iImage = imageIndex;
                item.pszText = pe.szExeFile;
                int row = ListView_InsertItem(hList, &item);

                // Add PID
                WCHAR pidStr[16];
                wsprintfW(pidStr, L"%u", pe.th32ProcessID);
                ListView_SetItemText(hList, row, 1, pidStr);

                // Restore selection if PID matches
                if (selectedPid[0] && wcscmp(pidStr, selectedPid) == 0) {
                    newSelectedIndex = row;
                }
                ++idx;
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);

    // --- Restore selection ---
    if (newSelectedIndex != -1) {
        ListView_SetItemState(hList, newSelectedIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }

    // --- Restore exact scroll position using actual row height ---
    int newTopIndex = (int)SendMessage(hList, LVM_GETTOPINDEX, 0, 0);
    int maxTop = max(0, idx - 1);
    int restoreTop = min(oldTopIndex, maxTop);
    int delta = restoreTop - newTopIndex;
    if (delta != 0 && rowHeight > 0) {
        SendMessage(hList, LVM_SCROLL, 0, delta * rowHeight);
    }

    // --- Re-enable redraw and force repaint ---
    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hList, NULL, TRUE);
}

void RunWinHiderCommand(const std::wstring& args, HWND hWnd, bool use64bit)
{
    // Extract PID from args
    size_t lastSpace = args.find_last_of(L' ');
    DWORD pid = _wtoi(args.substr(lastSpace + 1).c_str());

    // Always update both flags, not just one
    auto& state = hiddenProcesses[pid];
    if (args.find(L"--hide") == 0)
        state.screenShareHidden = true;
    else if (args.find(L"--unhide") == 0)
        state.screenShareHidden = false;
    else if (args.find(L"--hidetask") == 0)
        state.taskbarHidden = true;
    else if (args.find(L"--unhidetask") == 0)
        state.taskbarHidden = false;

    // Remove from map if neither hidden
    if (!state.screenShareHidden && !state.taskbarHidden)
        hiddenProcesses.erase(pid);

    // Execute command
    std::wstring exe = use64bit ? L"WinHider.exe" : L"WinHider_32bit.exe";
    std::wstring cmd = exe + L" " + args;
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessW(NULL, &cmd[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        InvalidateRect(hWnd, NULL, TRUE);
    }
    else {
        MessageBoxW(hWnd, (L"Failed to launch " + exe + L". Is it in the same directory?").c_str(), L"Error", MB_ICONERROR);
    }
}

//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM) //  PURPOSE: Processes messages for the main window.
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_NOTIFY - Handle notifications from controls
//  WM_DESTROY  - post a quit message and return
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hList, hBtnHide, hBtnUnhide, hBtnHideTask, hBtnUnhideTask, hBtnRefresh, hRadio64, hRadio32, hChkAutoRefresh, hBtnKill;
    static HWND hGroupBox1, hGroupBox2;
    static HFONT hFontDefault, hFontButton, hFontTitle; // Font handles

    switch (message)
    {
    case WM_CREATE:
    {
        // Create modern fonts
        // Get system DPI for proper scaling
        HDC hdc = GetDC(hWnd);
        int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(hWnd, hdc);

        // Create Segoe UI font (modern Windows font) - Default size
        hFontDefault = CreateFontW(
            -MulDiv(9, dpi, 72),    // Height (9pt)
            0,                      // Width (auto)
            0,                      // Escapement
            0,                      // Orientation
            FW_NORMAL,              // Weight
            FALSE,                  // Italic
            FALSE,                  // Underline
            FALSE,                  // StrikeOut
            DEFAULT_CHARSET,        // CharSet
            OUT_DEFAULT_PRECIS,     // OutPrecision
            CLIP_DEFAULT_PRECIS,    // ClipPrecision
            CLEARTYPE_QUALITY,      // Quality (important for modern look)
            DEFAULT_PITCH | FF_DONTCARE, // PitchAndFamily
            L"Segoe UI"             // Font name
        );

        // Create slightly larger font for buttons
        hFontButton = CreateFontW(
            -MulDiv(10, dpi, 72),   // Height (10pt)
            0, 0, 0,
            FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI"
        );

        // Create bold font for group box titles
        hFontTitle = CreateFontW(
            -MulDiv(9, dpi, 72),    // Height (9pt)
            0, 0, 0,
            FW_SEMIBOLD,            // Semi-bold weight
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI"
        );

        RECT rc;
        GetClientRect(hWnd, &rc);

        // Modern spacing constants
        const int MARGIN = 20;
        const int RADIO_HEIGHT = 25;
        const int GROUP_GAP = 30;
        const int BUTTON_HEIGHT = 40;
        const int BUTTON_WIDTH = 160;
        const int BUTTON_SPACING = 15;

        // Radio buttons for bitness selection
        hRadio64 = CreateWindowW(L"BUTTON", L"Show 64-bit Processes",
            WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
            MARGIN, MARGIN, 200, RADIO_HEIGHT, hWnd, (HMENU)IDC_RADIO_64BIT, hInst, NULL);
        SendMessageW(hRadio64, WM_SETFONT, (WPARAM)hFontDefault, TRUE);

        hRadio32 = CreateWindowW(L"BUTTON", L"Show 32-bit Processes",
            WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
            MARGIN + 220, MARGIN, 200, RADIO_HEIGHT, hWnd, (HMENU)IDC_RADIO_32BIT, hInst, NULL);
        SendMessageW(hRadio32, WM_SETFONT, (WPARAM)hFontDefault, TRUE);

        SendMessageW(hRadio64, BM_SETCHECK, BST_CHECKED, 0);

        // ListView with modern styling
        int listTop = MARGIN + RADIO_HEIGHT + 20;
        int listHeight = rc.bottom - listTop - 180;

        // ListView with modern styling
        hList = CreateWindowW(WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_BORDER,
            MARGIN, listTop, rc.right - (MARGIN * 2), listHeight,
            hWnd, (HMENU)IDC_PROCESS_LIST, hInst, NULL);

		// Enable custom draw and other features in ListView
        ListView_SetExtendedListViewStyle(hList,
            LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

        // Set font for ListView
        SendMessageW(hList, WM_SETFONT, (WPARAM)hFontDefault, TRUE);

        // Enable modern list view features
        ListView_SetExtendedListViewStyle(hList,
            LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

        // Create ImageList for process icons
        g_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 100, 100);
        ListView_SetImageList(hList, g_hImageList, LVSIL_SMALL);

		// Checkbox for auto-refresh
        hChkAutoRefresh = CreateWindowW(L"BUTTON", L"Auto Refresh List",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            MARGIN + 450, MARGIN, 140, 25, hWnd, (HMENU)IDC_CHK_AUTOREFRESH, hInst, NULL);
        SendMessageW(hChkAutoRefresh, WM_SETFONT, (WPARAM)hFontDefault, TRUE);
        SendMessageW(hChkAutoRefresh, BM_SETCHECK, BST_UNCHECKED, 0);
        autoRefreshEnabled = false;

        // Add columns
        LVCOLUMNW col = { 0 };
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        col.fmt = LVCFMT_LEFT | LVCFMT_IMAGE;
        col.pszText = (LPWSTR)L"Process Name";
        col.cx = 300;
        ListView_InsertColumn(hList, 0, &col);

        col.pszText = (LPWSTR)L"PID";
        col.cx = 100;
        ListView_InsertColumn(hList, 1, &col);

        // Group boxes with modern fonts
        int groupBoxTop = listTop + listHeight + GROUP_GAP;
        int groupBoxHeight = 100;

        hGroupBox1 = CreateWindowW(L"BUTTON", L"Screen Share Control",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            MARGIN, groupBoxTop, 200, groupBoxHeight,
            hWnd, NULL, hInst, NULL);
        SendMessageW(hGroupBox1, WM_SETFONT, (WPARAM)hFontTitle, TRUE);

        hGroupBox2 = CreateWindowW(L"BUTTON", L"Taskbar Control",
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            MARGIN + 220, groupBoxTop, 200, groupBoxHeight,
            hWnd, NULL, hInst, NULL);
        SendMessageW(hGroupBox2, WM_SETFONT, (WPARAM)hFontTitle, TRUE);

        // Buttons with modern fonts
        int btnStartY = groupBoxTop + 25;

        hBtnHide = CreateWindowW(L"BUTTON", L"Hide Screenshare",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            MARGIN + 10, btnStartY, BUTTON_WIDTH, BUTTON_HEIGHT,
            hWnd, (HMENU)IDC_BTN_HIDE, hInst, NULL);
        SendMessageW(hBtnHide, WM_SETFONT, (WPARAM)hFontButton, TRUE);

        hBtnUnhide = CreateWindowW(L"BUTTON", L"Unhide Screenshare",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            MARGIN + 10, btnStartY + BUTTON_HEIGHT + 10, BUTTON_WIDTH, BUTTON_HEIGHT,
            hWnd, (HMENU)IDC_BTN_UNHIDE, hInst, NULL);
        SendMessageW(hBtnUnhide, WM_SETFONT, (WPARAM)hFontButton, TRUE);

        hBtnHideTask = CreateWindowW(L"BUTTON", L"Hide Taskbar",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            MARGIN + 230, btnStartY, BUTTON_WIDTH, BUTTON_HEIGHT,
            hWnd, (HMENU)IDC_BTN_HIDETASK, hInst, NULL);
        SendMessageW(hBtnHideTask, WM_SETFONT, (WPARAM)hFontButton, TRUE);

        hBtnUnhideTask = CreateWindowW(L"BUTTON", L"Unhide Taskbar",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            MARGIN + 230, btnStartY + BUTTON_HEIGHT + 10, BUTTON_WIDTH, BUTTON_HEIGHT,
            hWnd, (HMENU)IDC_BTN_UNHIDETASK, hInst, NULL);
        SendMessageW(hBtnUnhideTask, WM_SETFONT, (WPARAM)hFontButton, TRUE);

        hBtnRefresh = CreateWindowW(L"BUTTON", L"↻ Refresh List",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            MARGIN + 450, btnStartY - 10, 120, BUTTON_HEIGHT,
            hWnd, (HMENU)IDC_BTN_REFRESH, hInst, NULL);
        SendMessageW(hBtnRefresh, WM_SETFONT, (WPARAM)hFontButton, TRUE);

        hBtnKill = CreateWindowW(L"BUTTON", L"🗙 Kill Process",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            MARGIN + 450, btnStartY + BUTTON_HEIGHT + 20, 120, BUTTON_HEIGHT,
            hWnd, (HMENU)IDC_BTN_KILL, hInst, NULL);
        SendMessageW(hBtnKill, WM_SETFONT, (WPARAM)hFontButton, TRUE);

        PopulateProcessList(hList, show64bit);
    }
    break;

    // Refresh process list with set timer interval
    case WM_TIMER:
        if (wParam == IDT_PROCESS_REFRESH_TIMER && autoRefreshEnabled)
            PopulateProcessList(hList, show64bit);
        break;

    case WM_GETMINMAXINFO:
    {
		// Set minimum window size
        MINMAXINFO* pMinMax = (MINMAXINFO*)lParam;
        pMinMax->ptMinTrackSize.x = 630; // Minimum width
        pMinMax->ptMinTrackSize.y = 400; // Minimum height
        return 0;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);

        // Handle commands that DO NOT require a process selection first
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDC_BTN_REFRESH:
            PopulateProcessList(hList, show64bit);
            break;
        case IDC_RADIO_64BIT:
            show64bit = true;
            SendMessageW(hRadio64, BM_SETCHECK, BST_CHECKED, 0);
            SendMessageW(hRadio32, BM_SETCHECK, BST_UNCHECKED, 0);
            PopulateProcessList(hList, show64bit);
            break;
        case IDC_RADIO_32BIT:
            show64bit = false;
            SendMessageW(hRadio64, BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessageW(hRadio32, BM_SETCHECK, BST_CHECKED, 0);
            PopulateProcessList(hList, show64bit);
            break;
        case IDC_CHK_AUTOREFRESH:
            autoRefreshEnabled = (SendMessageW((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED);
            if (autoRefreshEnabled) {
                SetTimer(hWnd, IDT_PROCESS_REFRESH_TIMER, g_processRefreshIntervalMs, NULL);
            }
            else {
                KillTimer(hWnd, IDT_PROCESS_REFRESH_TIMER);
            }
            break;

        case IDC_BTN_KILL:
        {
            int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (sel == -1) {
                MessageBoxW(hWnd, L"Please select a process to kill.", L"No Process Selected", MB_ICONINFORMATION | MB_OK);
                return 0;
            }
            WCHAR pidStr[16] = { 0 };
            ListView_GetItemText(hList, sel, 1, pidStr, 16);
            DWORD pid = _wtoi(pidStr);

            if (MessageBoxW(hWnd, L"Are you sure you want to kill the selected process?", L"Confirm Kill", MB_ICONWARNING | MB_YESNO) == IDYES) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                if (hProcess) {
                    if (TerminateProcess(hProcess, 1)) {
                        MessageBoxW(hWnd, L"Process terminated.", L"Success", MB_ICONINFORMATION);
                        PopulateProcessList(hList, show64bit);
                    }
                    else {
                        MessageBoxW(hWnd, L"Failed to terminate process.", L"Error", MB_ICONERROR);
                    }
                    CloseHandle(hProcess);
                }
                else {
                    MessageBoxW(hWnd, L"Failed to open process for termination.", L"Error", MB_ICONERROR);
                }
            }
            return 0;
        }

        default:
            // If it's not one of the above commands, it might require a process selection.
            // We will handle these below.
            break; // Break from this inner switch, but not the outer WM_COMMAND case
        }

        // Now, handle commands that DO require a process selection
        // Check if the current command is one of the buttons that need a selection
        if (wmId == IDC_BTN_HIDE ||
            wmId == IDC_BTN_UNHIDE ||
            wmId == IDC_BTN_HIDETASK ||
            wmId == IDC_BTN_UNHIDETASK)
        {
            // Get selected process
            int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (sel == -1) {
                MessageBoxW(hWnd, LR"delimiter(Please Select a Process form List to take actions on)delimiter", L"No Process Selected",
                    MB_ICONINFORMATION | MB_OK);
                // Important: If no process is selected, stop processing this command
                // by returning 0 or breaking from the case, depending on the context.
                // Returning 0 indicates the message was handled.
                return 0;
            }

            // If a process IS selected, proceed with the action
            WCHAR procName[260] = { 0 }, pidStr[16] = { 0 };
            ListView_GetItemText(hList, sel, 0, procName, 260);
            ListView_GetItemText(hList, sel, 1, pidStr, 16);
            std::wstring arg = pidStr;

            switch (wmId)
            {
            case IDC_BTN_HIDE:
                RunWinHiderCommand(L"--hide " + arg, hWnd, show64bit);
                break;
            case IDC_BTN_UNHIDE:
                RunWinHiderCommand(L"--unhide " + arg, hWnd, show64bit);
                break;
            case IDC_BTN_HIDETASK:
                RunWinHiderCommand(L"--hidetask " + arg, hWnd, show64bit);
                break;
            case IDC_BTN_UNHIDETASK:
                RunWinHiderCommand(L"--unhidetask " + arg, hWnd, show64bit);
                break;
            }
        }
        else
        {
            // If the command was not handled in the first switch
            // and it's not one of the buttons requiring selection,
            // let the default window procedure handle it.
            return DefWindowProc(hWnd, message, wParam, lParam);
        }

        // Return 0 to indicate the message was handled
        return 0;
    }
    break; // Break from the WM_COMMAND case


    case WM_SIZE:
        if (hList) {
            RECT rc;
            GetClientRect(hWnd, &rc);

            const int MARGIN = 20;
            const int RADIO_HEIGHT = 25;
            const int GROUP_GAP = 30;
            const int BUTTON_HEIGHT = 40;
            const int BUTTON_WIDTH = 160;

            MoveWindow(hRadio64, MARGIN, MARGIN, 200, RADIO_HEIGHT, TRUE);
            MoveWindow(hRadio32, MARGIN + 220, MARGIN, 200, RADIO_HEIGHT, TRUE);

            int listTop = MARGIN + RADIO_HEIGHT + 20;
            int listHeight = rc.bottom - listTop - 180;
            MoveWindow(hList, MARGIN, listTop, rc.right - (MARGIN * 2), listHeight, TRUE);

            int groupBoxTop = listTop + listHeight + GROUP_GAP;
            int groupBoxHeight = 100;

            MoveWindow(hGroupBox1, MARGIN, groupBoxTop, 200, groupBoxHeight, TRUE);
            MoveWindow(hGroupBox2, MARGIN + 220, groupBoxTop, 200, groupBoxHeight, TRUE);

            int btnStartY = groupBoxTop + 25;

            MoveWindow(hBtnHide, MARGIN + 10, btnStartY, BUTTON_WIDTH, BUTTON_HEIGHT, TRUE);
            MoveWindow(hBtnUnhide, MARGIN + 10, btnStartY + BUTTON_HEIGHT + 10, BUTTON_WIDTH, BUTTON_HEIGHT, TRUE);
            MoveWindow(hBtnHideTask, MARGIN + 230, btnStartY, BUTTON_WIDTH, BUTTON_HEIGHT, TRUE);
            MoveWindow(hBtnUnhideTask, MARGIN + 230, btnStartY + BUTTON_HEIGHT + 10, BUTTON_WIDTH, BUTTON_HEIGHT, TRUE);
            MoveWindow(hBtnRefresh, MARGIN + 450, btnStartY + 20, 120, BUTTON_HEIGHT, TRUE);
            MoveWindow(hBtnKill, MARGIN + 450, btnStartY + BUTTON_HEIGHT + 20, 120, BUTTON_HEIGHT, TRUE);
            MoveWindow(hChkAutoRefresh, MARGIN + 450, MARGIN, 140, 25, TRUE); 

        }
        break;

    case WM_NOTIFY:
    {
        LPNMHDR nmhdr = (LPNMHDR)lParam;
        if (nmhdr->hwndFrom == hList && nmhdr->code == NM_CUSTOMDRAW)
        {
            LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

            switch (lplvcd->nmcd.dwDrawStage)
            {
            case CDDS_PREPAINT:
                return CDRF_NOTIFYITEMDRAW;

            case CDDS_ITEMPREPAINT:
            {
                // Get the PID 
                WCHAR pidStr[16] = { 0 }; // Initialize the buffer with zeros to ensure null-termination
                ListView_GetItemText(hList, lplvcd->nmcd.dwItemSpec, 1, pidStr, _countof(pidStr)); // Use _countof for buffer size
                pidStr[_countof(pidStr) - 1] = L'\0';  // Manually null-terminate the buffer just in case ListView_GetItemText didn't
                DWORD pid = _wtoi(pidStr);

                auto it = hiddenProcesses.find(pid);
                if (it != hiddenProcesses.end())
                {
                    // Set different colors based on hidden state
                    if (it->second.screenShareHidden && it->second.taskbarHidden)
                        lplvcd->clrTextBk = RGB(144, 238, 144);  // Green for both hidden
                    else if (it->second.screenShareHidden)
                        lplvcd->clrTextBk = RGB(255, 200, 200);  // Red for screenshare hidden
                    else if (it->second.taskbarHidden)
                        lplvcd->clrTextBk = RGB(173, 216, 230);  // Blue for taskbar hidden
                }
                return CDRF_DODEFAULT;
            }
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code that uses hdc here...
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
		// Clean up the image list if it was created
        if (g_hImageList) ImageList_Destroy(g_hImageList);

        // Clean up font objects
        if (hFontDefault) DeleteObject(hFontDefault);
        if (hFontButton) DeleteObject(hFontButton);
        if (hFontTitle) DeleteObject(hFontTitle);

        KillTimer(hWnd, IDT_PROCESS_REFRESH_TIMER);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}