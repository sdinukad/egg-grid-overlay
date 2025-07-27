/**
 * @file grid_overlay.cpp
 * @brief A simple, lightweight, and resizable grid overlay for Windows.
 *
 * This application creates a transparent, click-through window with a grid overlay.
 * The user can toggle a "resize mode" with a global hotkey (Ctrl+Alt+G) to
 * move and resize the window. The window's last position is saved to the
 * Windows Registry and restored on the next launch.
 */

#include <windows.h>
#include <shellapi.h>
#include "resources.h"

//--------------------------------------------------------------------------------------
// Global Variables and Constants
//--------------------------------------------------------------------------------------
HINSTANCE g_hInstance = NULL;
HWND g_hWnd = NULL;
bool g_isResizeMode = false;
RECT g_windowRect = {100, 100, 900, 600}; // Default window position and size.

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
 * @brief Renders the grid lines onto the specified device context.
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
}

/**
 * @brief Switches the window to an interactive, non-click-through resize mode.
 *
 * This function makes the window solid and clickable by switching its transparency
 * mode from color-keying to alpha blending. It also displays standard window
 * borders and a title bar for easy manipulation.
 * @param hwnd Handle to the main window.
 */
void EnterResizeMode(HWND hwnd) {
    g_isResizeMode = true;

    // Switch to alpha blending to make the window solid and interactive.
    // 254 is nearly opaque, ensuring all mouse events are captured.
    SetLayeredWindowAttributes(hwnd, 0, 254, LWA_ALPHA);

    // Remove the click-through style and add standard window decorations.
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TOPMOST);
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX);
    SetWindowText(hwnd, L"Resize Mode (Press Ctrl+Alt+G or ESC to lock)");

    // Force the window to repaint and update its frame styles.
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
    SetForegroundWindow(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
}

/**
 * @brief Switches the window back to a transparent, click-through overlay mode.
 *
 * This function restores the click-through capability by switching the transparency
 * mode back to color-keying. It removes the window borders and saves the new
 * size and position.
 * @param hwnd Handle to the main window.
 */
void ExitResizeMode(HWND hwnd) {
    g_isResizeMode = false;
    GetWindowRect(hwnd, &g_windowRect);

    // Switch back to color-key transparency to make the background invisible
    // and restore click-through behavior for the empty areas.
    SetLayeredWindowAttributes(hwnd, TRANSPARENT_COLOR, 0, LWA_COLORKEY);

    // Re-apply the click-through style and remove window decorations.
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST);
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    SetWindowText(hwnd, APP_TITLE);
    
    // Apply the new window position and force the frame to update.
    SetWindowPos(hwnd, HWND_TOPMOST,
                 g_windowRect.left, g_windowRect.top,
                 g_windowRect.right - g_windowRect.left, g_windowRect.bottom - g_windowRect.top,
                 SWP_FRAMECHANGED);
    
    SaveSettings();
}

/**
 * @brief Adds an icon to the system tray.
 * @param hwnd Handle to the window that will receive tray icon notifications.
 */
void AddTrayIcon(HWND hwnd) {
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_APP_TRAY_MSG;
    nid.hIcon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    wcscpy_s(nid.szTip, L"Grid Overlay (Ctrl+Alt+G to resize)");

    Shell_NotifyIcon(NIM_ADD, &nid);

    // Set the icon version to receive modern, reliable mouse messages.
    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

/**
 * @brief Removes the icon from the system tray.
 * @param hwnd Handle to the window associated with the tray icon.
 */
void RemoveTrayIcon(HWND hwnd) {
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

/**
 * @brief Saves the window's last known position and size to the registry.
 */
void SaveSettings() {
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\SimpleGridOverlay", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        GetWindowRect(g_hWnd, &g_windowRect);
        RegSetValueEx(hKey, L"windowRect", 0, REG_BINARY, (const BYTE*)&g_windowRect, sizeof(g_windowRect));
        RegCloseKey(hKey);
    }
}

/**
 * @brief Loads the window's last known position and size from the registry.
 */
void LoadSettings() {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\SimpleGridOverlay", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwSize = sizeof(g_windowRect);
        RegGetValue(hKey, NULL, L"windowRect", RRF_RT_REG_BINARY, NULL, &g_windowRect, &dwSize);
        RegCloseKey(hKey);
    }
}

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

        case WM_HOTKEY: 
            if (wParam == RESIZE_HOTKEY_ID) {
                if (g_isResizeMode) {
                    ExitResizeMode(hwnd);
                } else {
                    EnterResizeMode(hwnd);
                }
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

            // Set the background color based on the current mode.
            // In resize mode, the background is a standard system color.
            // In overlay mode, it's the transparent color key.
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
                    if (g_isResizeMode) {
                        ExitResizeMode(hwnd);
                    } else {
                        EnterResizeMode(hwnd);
                    }
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

    // Register the window class.
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    RegisterClassEx(&wc);

    // Create the main window.
    HWND hWnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST, // Initial styles for click-through overlay
        CLASS_NAME,
        APP_TITLE,
        WS_POPUP, // Borderless window
        g_windowRect.left, g_windowRect.top,
        g_windowRect.right - g_windowRect.left, g_windowRect.bottom - g_windowRect.top,
        NULL, NULL, hInstance, NULL);

    if (hWnd == NULL) {
        return 0;
    }
    
    // Register the global hotkey for toggling resize mode.
    RegisterHotKey(hWnd, RESIZE_HOTKEY_ID, MOD_CONTROL | MOD_ALT, 'G');
    
    // Set the initial transparency mode.
    SetLayeredWindowAttributes(hWnd, TRANSPARENT_COLOR, 0, LWA_COLORKEY);
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Main message loop.
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}