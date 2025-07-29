/**
 * @file grid_overlay.cpp
 * @brief A simple, lightweight, and resizable grid overlay for Windows.
 *
 * This application creates a transparent, click-through window with a grid overlay.
 * The user can toggle a "resize mode" with a global hotkey (Ctrl+Alt+G) to
 * move, resize, and place a custom marker dot. The window's last position and
 * dot location are saved to the Windows Registry.
 */

#include <windows.h>
#include <shellapi.h>
#include <wchar.h>
#include "resources.h"

//--------------------------------------------------------------------------------------
// Global Variables and Constants
//--------------------------------------------------------------------------------------
HINSTANCE g_hInstance = NULL;
HWND g_hWnd = NULL;
bool g_isResizeMode = false;
RECT g_windowRect = {100, 100, 900, 600}; // Default window position and size.

// <<< NEW: Custom Dot variables >>>
POINT g_customDot = {0, 0};
bool g_isDotSet = false;

// Grid dimensions
const int g_cols = 10;
const int g_rows = 6;

// Application identifiers
const wchar_t CLASS_NAME[] = L"SimpleGridOverlayClass";
const wchar_t APP_TITLE[] = L"Grid Overlay";
const UINT WM_APP_TRAY_MSG = WM_APP + 1;
const int RESIZE_HOTKEY_ID = 1;

// The color used for the transparent background in overlay mode.
const COLORREF TRANSPARENT_COLOR = RGB(0, 0, 1);

//--------------------------------------------------------------------------------------
// Forward Declarations
//--------------------------------------------------------------------------------------
void AddTrayIcon(HWND hwnd);
void RemoveTrayIcon(HWND hwnd);
void EnterResizeMode(HWND hwnd);
void ExitResizeMode(HWND hwnd);
void SaveSettings();
void LoadSettings();
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/**
 * @brief Renders the grid, column numbers, and custom dot onto the device context.
 * @param hdc The device context to draw on.
 */
void DrawGrid(HDC hdc) {
    RECT clientRect;
    GetClientRect(g_hWnd, &clientRect);

    const int width = clientRect.right;
    const int height = clientRect.bottom;

    if (g_cols <= 0 || g_rows <= 0) {
        return;
    }

    const float cellWidth = (float)width / g_cols;
    const float cellHeight = (float)height / g_rows;

    // --- Grid Line Drawing ---
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(138, 43, 226));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    for (int i = 1; i < g_cols; ++i) {
        int x = (int)(i * cellWidth);
        MoveToEx(hdc, x, 0, NULL);
        LineTo(hdc, x, height);
    }
    for (int i = 1; i < g_rows; ++i) {
        int y = (int)(i * cellHeight);
        MoveToEx(hdc, 0, y, NULL);
        LineTo(hdc, width, y);
    }

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    // --- Number Drawing ---
    int fontHeight = (int)(cellHeight * 0.6);
    HFONT hFont = CreateFont(fontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    SetTextColor(hdc, RGB(192, 192, 192));
    SetBkMode(hdc, TRANSPARENT);

    wchar_t numberStr[4];
    for (int i = 0; i < g_cols; ++i) {
        swprintf(numberStr, 4, L"%d", i + 1);
        RECT cellRect = { (int)(i * cellWidth), 0, (int)((i + 1) * cellWidth), (int)cellHeight };
        DrawText(hdc, numberStr, -1, &cellRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);

    // --- <<< NEW: Custom Dot Drawing >>> ---
    if (g_isDotSet) {
        HBRUSH hDotBrush = CreateSolidBrush(RGB(255, 0, 0)); // Bright red brush
        HPEN hDotPen = CreatePen(PS_NULL, 0, 0); // No border for the dot
        
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hDotBrush);
        HPEN hOldDotPen = (HPEN)SelectObject(hdc, hDotPen);

        // Draw a circle (ellipse) with a 5-pixel radius centered on the stored point.
        Ellipse(hdc, g_customDot.x - 5, g_customDot.y - 5, g_customDot.x + 5, g_customDot.y + 5);

        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldDotPen);
        DeleteObject(hDotBrush);
        DeleteObject(hDotPen);
    }
}

/**
 * @brief Switches the window to an interactive, non-click-through resize mode.
 */
void EnterResizeMode(HWND hwnd) {
    g_isResizeMode = true;
    SetLayeredWindowAttributes(hwnd, 0, 254, LWA_ALPHA);
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TOPMOST);
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX);
    
    // <<< CHANGE: Updated instructions in title bar >>>
    SetWindowText(hwnd, L"Resize | L-Click: Place Dot | R-Click: Remove | ESC: Lock");

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
    SetForegroundWindow(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
}

/**
 * @brief Switches the window back to a transparent, click-through overlay mode.
 */
void ExitResizeMode(HWND hwnd) {
    g_isResizeMode = false;
    GetWindowRect(hwnd, &g_windowRect);
    SetLayeredWindowAttributes(hwnd, TRANSPARENT_COLOR, 0, LWA_COLORKEY);
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST);
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    SetWindowText(hwnd, APP_TITLE);
    SetWindowPos(hwnd, HWND_TOPMOST,
                 g_windowRect.left, g_windowRect.top,
                 g_windowRect.right - g_windowRect.left, g_windowRect.bottom - g_windowRect.top,
                 SWP_FRAMECHANGED);
    
    SaveSettings(); // Save all settings, including the dot's state.
}

/**
 * @brief Saves window position and custom dot state to the registry.
 */
void SaveSettings() {
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\SimpleGridOverlay", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        GetWindowRect(g_hWnd, &g_windowRect);
        RegSetValueEx(hKey, L"windowRect", 0, REG_BINARY, (const BYTE*)&g_windowRect, sizeof(g_windowRect));
        
        // <<< NEW: Save dot state and position >>>
        RegSetValueEx(hKey, L"isDotSet", 0, REG_DWORD, (const BYTE*)&g_isDotSet, sizeof(g_isDotSet));
        if (g_isDotSet) {
            RegSetValueEx(hKey, L"customDot", 0, REG_BINARY, (const BYTE*)&g_customDot, sizeof(g_customDot));
        }
        
        RegCloseKey(hKey);
    }
}

/**
 * @brief Loads window position and custom dot state from the registry.
 */
void LoadSettings() {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\SimpleGridOverlay", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwSizeRect = sizeof(g_windowRect);
        RegGetValue(hKey, NULL, L"windowRect", RRF_RT_REG_BINARY, NULL, &g_windowRect, &dwSizeRect);
        
        // <<< NEW: Load dot state and position >>>
        DWORD dwSizeBool = sizeof(g_isDotSet);
        RegGetValue(hKey, NULL, L"isDotSet", RRF_RT_DWORD, NULL, &g_isDotSet, &dwSizeBool);

        if (g_isDotSet) {
            DWORD dwSizePoint = sizeof(g_customDot);
            RegGetValue(hKey, NULL, L"customDot", RRF_RT_REG_BINARY, NULL, &g_customDot, &dwSizePoint);
        }
        
        RegCloseKey(hKey);
    }
}

// ... (AddTrayIcon and RemoveTrayIcon are unchanged) ...
void AddTrayIcon(HWND hwnd) { NOTIFYICONDATA nid = {}; nid.cbSize = sizeof(NOTIFYICONDATA); nid.hWnd = hwnd; nid.uID = 1; nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP; nid.uCallbackMessage = WM_APP_TRAY_MSG; nid.hIcon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE); wcscpy_s(nid.szTip, L"Grid Overlay (Ctrl+Alt+G to resize)"); Shell_NotifyIcon(NIM_ADD, &nid); nid.uVersion = NOTIFYICON_VERSION_4; Shell_NotifyIcon(NIM_SETVERSION, &nid); }
void RemoveTrayIcon(HWND hwnd) { NOTIFYICONDATA nid = {}; nid.cbSize = sizeof(NOTIFYICONDATA); nid.hWnd = hwnd; nid.uID = 1; Shell_NotifyIcon(NIM_DELETE, &nid); }


/**
 * @brief The main window procedure for handling window messages.
 */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            g_hWnd = hwnd;
            AddTrayIcon(hwnd);
            SetWindowPos(hwnd, HWND_TOPMOST, g_windowRect.left, g_windowRect.top, g_windowRect.right - g_windowRect.left, g_windowRect.bottom - g_windowRect.top, SWP_SHOWWINDOW);
            return 0;

        // <<< NEW: Handle mouse clicks for the custom dot >>>
        case WM_LBUTTONDOWN:
            if (g_isResizeMode) {
                g_customDot.x = LOWORD(lParam);
                g_customDot.y = HIWORD(lParam);
                g_isDotSet = true;
                InvalidateRect(hwnd, NULL, TRUE); // Force repaint to show the dot
            }
            return 0;

        case WM_RBUTTONDOWN:
            if (g_isResizeMode) {
                g_isDotSet = false;
                InvalidateRect(hwnd, NULL, TRUE); // Force repaint to remove the dot
            }
            return 0;

        case WM_HOTKEY: 
            if (wParam == RESIZE_HOTKEY_ID) {
                if (g_isResizeMode) ExitResizeMode(hwnd);
                else EnterResizeMode(hwnd);
            }
            return 0;

        case WM_KEYDOWN:
            if (g_isResizeMode && wParam == VK_ESCAPE) {
                ExitResizeMode(hwnd);
            }
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            COLORREF bgColor = g_isResizeMode ? GetSysColor(COLOR_3DFACE) : TRANSPARENT_COLOR;
            HBRUSH bgBrush = CreateSolidBrush(bgColor);
            FillRect(hdc, &ps.rcPaint, bgBrush);
            DeleteObject(bgBrush);
            DrawGrid(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_APP_TRAY_MSG:
            if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
                HMENU hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_TRAYMENU));
                if (hMenu) {
                    HMENU hSubMenu = GetSubMenu(hMenu, 0);
                    POINT pt;
                    GetCursorPos(&pt);
                    SetForegroundWindow(hwnd);
                    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
                    PostMessage(hwnd, WM_NULL, 0, 0);
                    DestroyMenu(hMenu);
                }
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_TRAY_EXIT:
                    DestroyWindow(hwnd);
                    break;
                case ID_TRAY_RESIZE:
                    if (g_isResizeMode) ExitResizeMode(hwnd);
                    else EnterResizeMode(hwnd);
                    break;
            }
            return 0;

        case WM_DESTROY:
            RemoveTrayIcon(hwnd);
            UnregisterHotKey(hwnd, RESIZE_HOTKEY_ID); 
            SaveSettings();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/**
 * @brief The main entry point for the application.
 */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    g_hInstance = hInstance;
    LoadSettings();

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassEx(&wc);

    HWND hWnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        CLASS_NAME, APP_TITLE, WS_POPUP,
        g_windowRect.left, g_windowRect.top,
        g_windowRect.right - g_windowRect.left, g_windowRect.bottom - g_windowRect.top,
        NULL, NULL, hInstance, NULL);

    if (hWnd == NULL) return 0;
    
    RegisterHotKey(hWnd, RESIZE_HOTKEY_ID, MOD_CONTROL | MOD_ALT, 'G');
    SetLayeredWindowAttributes(hWnd, TRANSPARENT_COLOR, 0, LWA_COLORKEY);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}