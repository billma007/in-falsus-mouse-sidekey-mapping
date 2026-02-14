#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>

// Link with common controls library
#pragma comment(lib, "comctl32.lib")

// Global variables
HINSTANCE g_hInst = NULL;
HWND g_hMainWnd = NULL;
HHOOK g_hMouseHook = NULL;

// UI Controls
HWND g_hCheckEnable = NULL;
HWND g_hCheckSwap = NULL;
HWND g_hHotkeyInput = NULL;
HWND g_hBtnSetHotkey = NULL;
HWND g_hStatusLabel = NULL;
HWND g_hBtnLang = NULL;      // Language toggle button
HWND g_hGroupHotkey = NULL;  // Hotkey group box
HWND g_hDescLabel = NULL;    // Description label
HWND g_hCheckMouseMap = NULL;// New Checkbox for Mouse Mapping

// State
bool g_enabled = true;
bool g_swapMapping = false;
bool g_mouseMap = false;     // False: Side->Key, True: Mouse->Key
bool g_isEnglish = false; // False = Chinese, True = English
bool g_btn1Pressed = false;
bool g_btn2Pressed = false;
bool g_lBtnPressed = false;
bool g_rBtnPressed = false;

// Hotkey
int g_hotkeyMod = MOD_CONTROL | MOD_ALT;
int g_hotkeyVk = 'H';
int g_hotkeyId = 1;

// Function prototypes
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LowLevelMouseProc(int, WPARAM, LPARAM);
void UpdateStatus();
void ToggleEnable();
void RegisterGlobalHotkey();
void ToggleLanguage();
void UpdateUIText();
void UpdateUIInteraction();

// Entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    g_hInst = hInstance;

    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    // Register window class
    const wchar_t CLASS_NAME[] = L"SideButtonMapperClass";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    // Create window
    // Calculate center position
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 450;
    int winH = 420;
    int x = (screenW - winW) / 2;
    int y = (screenH - winH) / 2;

    g_hMainWnd = CreateWindowEx(
        0, CLASS_NAME, L"in falsus 侧键映射工具 - by BillMa007",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, winW, winH,
        NULL, NULL, hInstance, NULL
    );

    if (g_hMainWnd == NULL) {
        return 0;
    }

    // Install Mouse Hook
    g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0);
    if (!g_hMouseHook) {
        MessageBox(NULL, L"无法安装鼠标钩子！", L"错误", MB_ICONERROR);
        return 0;
    }

    RegisterGlobalHotkey();
    UpdateUIText(); // Initialize text
    UpdateStatus();
    UpdateUIInteraction();

    ShowWindow(g_hMainWnd, nCmdShow);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    if (g_hMouseHook) UnhookWindowsHookEx(g_hMouseHook);
    UnregisterHotKey(g_hMainWnd, g_hotkeyId);

    return 0;
}

// Helper to send key input
// DO NOT use KEYEVENTF_SCANCODE here if we want mapped keys, but for games SCANCODE is often required.
// MapVirtualKey(vk, 0) gets the scan code.
void SendKey(WORD vk, bool down) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.wScan = MapVirtualKey(vk, 0); // Hardware scan code
    input.ki.dwFlags = (down ? 0 : KEYEVENTF_KEYUP) | KEYEVENTF_SCANCODE; // Use scan code for games
    SendInput(1, &input, sizeof(INPUT));
}

// Mouse Hook Procedure
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode < 0 || !g_enabled) {
        return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
    }

    MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;

    // Handle Left/Right Mouse Button events if g_mouseMap is TRUE
    if (g_mouseMap && (wParam == WM_LBUTTONDOWN || wParam == WM_LBUTTONUP || wParam == WM_RBUTTONDOWN || wParam == WM_RBUTTONUP)) {
        // Prevent recursive loop if injected event trigger hook
        if (pMouse->flags & LLMHF_INJECTED) {
            return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
        }

        bool isLeft = (wParam == WM_LBUTTONDOWN || wParam == WM_LBUTTONUP);
        bool isDown = (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN);

        WORD targetVk = 0;

        if (isLeft) {
            // Map Left Mouse -> Shift (or Space if swapped)
            targetVk = g_swapMapping ? VK_SPACE : VK_LSHIFT;
            
            if (isDown) {
                if (g_lBtnPressed) return 1;
                g_lBtnPressed = true;
            } else {
                if (!g_lBtnPressed) return 1;
                g_lBtnPressed = false;
            }
        } else {
            // Right Mouse
            targetVk = g_swapMapping ? VK_LSHIFT : VK_SPACE;
            
            if (isDown) {
                if (g_rBtnPressed) return 1;
                g_rBtnPressed = true;
            } else {
                if (!g_rBtnPressed) return 1;
                g_rBtnPressed = false;
            }
        }

        if (targetVk != 0) {
            SendKey(targetVk, isDown);
            return 1; // Block original Mouse Click
        }
    }

    if (wParam == WM_XBUTTONDOWN || wParam == WM_XBUTTONUP) {
        // If g_mouseMap is TRUE, we must DISABLE normal XButton mapping.
        // User Requirement: "Must cancel previous mapping"
        if (g_mouseMap) {
             return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
        }

        // Determine which X button (1 or 2)
        bool isX1 = (HIWORD(pMouse->mouseData) & XBUTTON1);
        bool isX2 = (HIWORD(pMouse->mouseData) & XBUTTON2);
        bool isDown = (wParam == WM_XBUTTONDOWN);

        WORD targetVk = 0;

        if (isX1) {
            targetVk = g_swapMapping ? VK_SPACE : VK_LSHIFT;
            // Prevent auto-repeat logic interference if we handle state manually, 
            // but Windows hook gives direct down/up events.
            if (isDown) {
                if (g_btn1Pressed) return 1; // Ignore repeated down events from driver if any (debounce)
                g_btn1Pressed = true;
            }
            else {
                if (!g_btn1Pressed) return 1;
                g_btn1Pressed = false;
            }
        }
        else if (isX2) {
            targetVk = g_swapMapping ? VK_LSHIFT : VK_SPACE;
            if (isDown) {
                if (g_btn2Pressed) return 1;
                g_btn2Pressed = true;
            }
            else {
                if (!g_btn2Pressed) return 1;
                g_btn2Pressed = false;
            }
        }

        if (targetVk != 0) {
            SendKey(targetVk, isDown);
            return 1; // Block the original XBUTTON event
        }
    }

    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

// Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
    {
        // Define fonts
        HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");

        // UI Creation
        int y = 10;
        
        // Language Toggle Button (Top Right corner)
        // Explicitly set text here to ensure visibility
        g_hBtnLang = CreateWindow(L"BUTTON", L"语言/Lang",
            WS_VISIBLE | WS_CHILD,
            340, y, 90, 25, hwnd, (HMENU)104, g_hInst, NULL);
        SendMessage(g_hBtnLang, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        // Checkbox: Enable
        g_hCheckEnable = CreateWindow(L"BUTTON", L"",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            20, y, 300, 30, hwnd, (HMENU)101, g_hInst, NULL);
        SendMessage(g_hCheckEnable, BM_SETCHECK, BST_CHECKED, 0);
        SendMessage(g_hCheckEnable, WM_SETFONT, (WPARAM)hFont, TRUE);
        y += 35;

        // Checkbox: Swap
        g_hCheckSwap = CreateWindow(L"BUTTON", L"",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            20, y, 300, 30, hwnd, (HMENU)102, g_hInst, NULL);
        SendMessage(g_hCheckSwap, WM_SETFONT, (WPARAM)hFont, TRUE);
        y += 35;

        // Checkbox: Mouse Map (New)
        g_hCheckMouseMap = CreateWindow(L"BUTTON", L"",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            20, y, 400, 30, hwnd, (HMENU)105, g_hInst, NULL);
        SendMessage(g_hCheckMouseMap, WM_SETFONT, (WPARAM)hFont, TRUE);
        y += 40;

        // Group Box for Hotkey
        g_hGroupHotkey = CreateWindow(L"BUTTON", L"",
            WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
            15, y, 350, 70, hwnd, NULL, g_hInst, NULL);
        SendMessage(g_hGroupHotkey, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        // Hotkey Control
        g_hHotkeyInput = CreateWindow(HOTKEY_CLASS, L"",
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            25, y + 25, 200, 25, hwnd, NULL, g_hInst, NULL);
        // Set default: Ctrl + Alt + H
        SendMessage(g_hHotkeyInput, HKM_SETRULES, HKCOMB_NONE, MOD_WIN); 
        SendMessage(g_hHotkeyInput, HKM_SETHOTKEY, MAKEWORD('H', HOTKEYF_CONTROL | HOTKEYF_ALT), 0);
        SendMessage(g_hHotkeyInput, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Set Button
        g_hBtnSetHotkey = CreateWindow(L"BUTTON", L"",
            WS_VISIBLE | WS_CHILD,
            240, y + 24, 80, 27, hwnd, (HMENU)103, g_hInst, NULL);
        SendMessage(g_hBtnSetHotkey, WM_SETFONT, (WPARAM)hFont, TRUE);
        y += 85;

        // Status Label
        g_hStatusLabel = CreateWindow(L"STATIC", L"",
            WS_VISIBLE | WS_CHILD,
            20, y, 350, 25, hwnd, NULL, g_hInst, NULL);
        SendMessage(g_hStatusLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

        y += 35;
        // Description
        g_hDescLabel = CreateWindow(L"STATIC", L"",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            20, y, 400, 160, hwnd, NULL, g_hInst, NULL);
        SendMessage(g_hDescLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

        // UpdateUIText will be called in wWinMain after g_hMainWnd is assigned
    }
    return 0;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
            int id = LOWORD(wParam);
            if (id == 101) {
                g_enabled = (SendMessage(g_hCheckEnable, BM_GETCHECK, 0, 0) == BST_CHECKED);
                UpdateStatus();
                UpdateUIInteraction();
            }
            else if (id == 102) {
                g_swapMapping = (SendMessage(g_hCheckSwap, BM_GETCHECK, 0, 0) == BST_CHECKED);
                UpdateStatus();
            }
            else if (id == 105) { // Mouse Map Toggle
                g_mouseMap = (SendMessage(g_hCheckMouseMap, BM_GETCHECK, 0, 0) == BST_CHECKED);
                UpdateStatus();
            }
            else if (id == 103) {
                RegisterGlobalHotkey();
                if (g_isEnglish)
                    MessageBox(hwnd, L"Hotkey Updated!", L"Info", MB_OK | MB_ICONINFORMATION);
                else
                    MessageBox(hwnd, L"快捷键已更新", L"提示", MB_OK | MB_ICONINFORMATION);
            }
            else if (id == 104) { // Language toggle
                ToggleLanguage();
            }
        }
        return 0;

    case WM_HOTKEY:
        if (wParam == g_hotkeyId) {
            ToggleEnable();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void ToggleEnable() {
    g_enabled = !g_enabled;
    SendMessage(g_hCheckEnable, BM_SETCHECK, g_enabled ? BST_CHECKED : BST_UNCHECKED, 0);
    UpdateStatus();
    UpdateUIInteraction();
}

void UpdateStatus() {
    std::wstring text;
    if (g_isEnglish) {
        text = L"Status: ";
        text += g_enabled ? L"Active" : L"Disabled";
        text += L" | Swap: ";
        text += g_swapMapping ? L"Yes" : L"No";
        text += L" | Mode: ";
        text += g_mouseMap ? L"Mouse Button Map" : L"Side Key Map";
    } else {
        text = L"状态: ";
        text += g_enabled ? L"启用" : L"禁用";
        text += L" | 对调: ";
        text += g_swapMapping ? L"是" : L"否";
        text += L" | 模式: ";
        text += g_mouseMap ? L"鼠标按键映射" : L"侧键映射";
    }
    SetWindowText(g_hStatusLabel, text.c_str());
}

void UpdateUIInteraction() {
    BOOL bEnable = !g_enabled;
    // When mapping is enabled (especially mouse mapping), prevent hotkey changes
    // because clicking the controls might be difficult or cause issues.
    EnableWindow(g_hHotkeyInput, bEnable);
    EnableWindow(g_hBtnSetHotkey, bEnable);
}

void RegisterGlobalHotkey() {
    LRESULT hk = SendMessage(g_hHotkeyInput, HKM_GETHOTKEY, 0, 0);
    BYTE vk = LOBYTE(hk);
    BYTE mod = HIBYTE(hk);

    UINT fsModifiers = 0;
    if (mod & HOTKEYF_SHIFT) fsModifiers |= MOD_SHIFT;
    if (mod & HOTKEYF_CONTROL) fsModifiers |= MOD_CONTROL;
    if (mod & HOTKEYF_ALT) fsModifiers |= MOD_ALT;
    if (mod & HOTKEYF_EXT) fsModifiers |= MOD_WIN; 

    UnregisterHotKey(g_hMainWnd, 1);
    if (vk != 0) {
        if (!RegisterHotKey(g_hMainWnd, 1, fsModifiers, vk)) {
             if (g_isEnglish)
                 MessageBox(g_hMainWnd, L"Failed to register hotkey. It might be in use.", L"Error", MB_OK | MB_ICONERROR);
             else
                 MessageBox(g_hMainWnd, L"快捷键注册失败，可能被占用", L"错误", MB_OK | MB_ICONERROR);
        }
    }
}

void ToggleLanguage() {
    g_isEnglish = !g_isEnglish;
    UpdateUIText();
    UpdateStatus();
}

void UpdateUIText() {
    std::wstring title, btnLang, chkEnable, chkSwap, chkMouseMap, grpHotkey, btnSet, desc;
    
    if (g_isEnglish) {
        title = L"in falsus Side-Key Mapping Tool - by BillMa007";
        btnLang = L"中/EN";
        chkEnable = L"Enable Mapping";
        chkSwap = L"Swap Forward/Backward Keys";
        chkMouseMap = L"Map Left/Right Click to Shift/Space";
        grpHotkey = L"Toggle Hotkey";
        btnSet = L"Apply";
        desc = L"This tool maps mouse side-buttons to Left Shift and Space for the game 'in falsus'.\r\n"
               L"Unlike generic mapping tools, this uses Hardware Scan Codes for real-time mapping,\r\n"
               L"ensuring hold-actions work correctly (e.g. holding a key).\r\n"
                L"If you want to map the mouse left/right buttons instead of side keys, please check [Map Left/Right Click to Shift/Space].\r\n"
                L"After enabling mouse button mapping, please remember your toggle hotkey. In emergencies, you can use Alt+Tab to switch windows and then Alt+F4 to close the tool directly.\r\n"
               L"For questions, email: ma237103015@126.com or QQ: 36937975\r\n"
               L"Project: https://github.com/billma007/in-falsus-mouse-sidekey-mapping";
    } else {
        title = L"in falsus 侧键映射工具 - by BillMa007";
        btnLang = L"中/EN";
        chkEnable = L"启用替换效果";
        chkSwap = L"前键/后键效果对调";
        chkMouseMap = L"将鼠标左右键映射为Shift/空格";
        grpHotkey = L"快速开关快捷键";
        btnSet = L"应用";
        desc = L"本工具是用来在游戏in falsus中将左shift键和空格键映射到鼠标侧键的。\r\n"
               L"和一般的按键映射不同，本工具使用硬件扫描码 (Scan Codes)进行实时映射\r\n"
               L"以保证有长条的时候能够长按侧键保持。\r\n"
               L"如果不想映射侧键，而是映射鼠标左右键，请勾选[将鼠标左右键映射为Shift/空格]。\r\n"
               L"开启鼠标左右键映射后，请牢记你的开关快捷键。紧急情况可使用Alt+Tab切换窗口后Alt+F4直接关闭工具\r\n"
               L"如有问题请发邮件到ma237103015@126.com或者QQ36937975进行咨询。\r\n"
               L"项目地址 https://github.com/billma007/in-falsus-mouse-sidekey-mapping";
    }
    SetWindowText(g_hMainWnd, title.c_str());
    SetWindowText(g_hBtnLang, btnLang.c_str());
    SetWindowText(g_hCheckEnable, chkEnable.c_str());
    SetWindowText(g_hCheckSwap, chkSwap.c_str());
    SetWindowText(g_hCheckMouseMap, chkMouseMap.c_str());
    SetWindowText(g_hGroupHotkey, grpHotkey.c_str());
    SetWindowText(g_hBtnSetHotkey, btnSet.c_str());
    SetWindowText(g_hDescLabel, desc.c_str());
}
