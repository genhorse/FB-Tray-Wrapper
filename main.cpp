#include <windows.h>
#include <shellapi.h>
#include <wrl.h>
#include <WebView2.h>
#include "resource.h"

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Callback;

// ---------------------------------------------------------------
// Global Variables
// ---------------------------------------------------------------
HWND    g_hWnd      = nullptr;
ComPtr<ICoreWebView2Controller> g_controller;
ComPtr<ICoreWebView2>           g_webView;

// Storage for original window styles to handle transitions
LONG_PTR g_origStyle   = 0;
LONG_PTR g_origExStyle = 0;

// Custom message for tray icon interactions
constexpr UINT WM_TRAYICON   = WM_APP + 1;
constexpr UINT IDM_TRAY_SHOW = 40001;
constexpr UINT IDM_TRAY_EXIT = 40002;

NOTIFYICONDATA g_TrayIcon = {};

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void AddTrayIcon(HWND);
void RemoveTrayIcon();
void ShowMainWindow();
void HideMainWindow();

// ---------------------------------------------------------------
// Application Entry Point
// ---------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int)
{
    // Initialize COM for WebView2
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return -1;

    const wchar_t CLASS_NAME[] = L"FacebookWebView2Demo";

    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APPICON));
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassEx(&wc);

    // Create the main window (initially hidden)
    g_hWnd = CreateWindowEx(
        0, CLASS_NAME, L"WebView2 + Facebook demo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1024, 720,
        nullptr, nullptr, hInst, nullptr);

    if (!g_hWnd) { CoUninitialize(); return -1; }

    // Save styles immediately after creation
    g_origStyle   = GetWindowLongPtr(g_hWnd, GWL_STYLE);
    g_origExStyle = GetWindowLongPtr(g_hWnd, GWL_EXSTYLE);

    // Initial state: Show tray icon, hide main window
    ShowWindow(g_hWnd, SW_HIDE);
    UpdateWindow(g_hWnd); 

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    RemoveTrayIcon();
    g_controller.Reset();
    g_webView.Reset();
    CoUninitialize();
    return (int)msg.wParam;
}

// ---------------------------------------------------------------
// Window Procedure: Handles System and Custom Messages
// ---------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        AddTrayIcon(hWnd);

        // Initialize WebView2 Environment
        CreateCoreWebView2EnvironmentWithOptions(
            nullptr, nullptr, nullptr,
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [hWnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
                {
                    if (FAILED(result) || !env) return result;

                    env->CreateCoreWebView2Controller(
                        hWnd,
                        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                            [hWnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT
                            {
                                if (FAILED(result) || !controller) return result;

                                g_controller = controller;
                                g_controller->get_CoreWebView2(&g_webView);

                                // Set initial bounds based on client area
                                RECT rc; GetClientRect(g_hWnd, &rc);
                                g_controller->put_Bounds(rc);

                                // Navigate to Target URL
                                g_webView->Navigate(L"https://www.facebook.com");

                                // Keep it hidden on startup
                                HideMainWindow(); 
                                return S_OK;
                            }).Get());
                    return S_OK;
                }).Get());
        break;

    case WM_SIZE:
        // Update WebView2 bounds when window size changes
        if (wParam != SIZE_MINIMIZED && g_controller)
        {
            RECT rc; GetClientRect(g_hWnd, &rc);
            g_controller->put_Bounds(rc);
        }
        break;

    case WM_SYSCOMMAND:
        {
            const UINT cmd = static_cast<UINT>(wParam & 0xFFF0);
            // Intercept Minimize and Close buttons to hide in tray instead
            if (cmd == SC_MINIMIZE || cmd == SC_CLOSE)
            {
                HideMainWindow();
                return 0; // "Consume" the message
            }
        }
        break;

    case WM_CLOSE:
        HideMainWindow();
        return 0;

    case WM_TRAYICON:
        // Handle interactions with the system tray icon
        switch (lParam)
        {
        case WM_LBUTTONDBLCLK:
            ShowMainWindow();
            break;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            {
                POINT pt; GetCursorPos(&pt);
                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING, IDM_TRAY_SHOW, L"Show");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(hMenu, MF_STRING, IDM_TRAY_EXIT, L"Exit");
                SetForegroundWindow(hWnd); // Required to make menu disappear correctly
                TrackPopupMenuEx(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, hWnd, nullptr);
                DestroyMenu(hMenu);
            }
            break;
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_TRAY_SHOW:
            ShowMainWindow();
            break;
        case IDM_TRAY_EXIT:
            RemoveTrayIcon();
            DestroyWindow(g_hWnd);
            break;
        }
        return 0;

    case WM_DESTROY:
        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// ---------------------------------------------------------------
// Tray and Window Management Functions
// ---------------------------------------------------------------
void AddTrayIcon(HWND hWnd)
{
    ZeroMemory(&g_TrayIcon, sizeof(g_TrayIcon));
    g_TrayIcon.cbSize = sizeof(NOTIFYICONDATA);
    g_TrayIcon.hWnd   = hWnd;
    g_TrayIcon.uID    = 1;
    g_TrayIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_TrayIcon.uCallbackMessage = WM_TRAYICON;
    g_TrayIcon.hIcon  = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON));
    wcscpy_s(g_TrayIcon.szTip, L"WebView2 - Facebook Wrapper");
    Shell_NotifyIcon(NIM_ADD, &g_TrayIcon);
}

void RemoveTrayIcon() { Shell_NotifyIcon(NIM_DELETE, &g_TrayIcon); }

void ShowMainWindow()
{
    // Restore original window style
    SetWindowLongPtr(g_hWnd, GWL_STYLE,   g_origStyle);
    SetWindowLongPtr(g_hWnd, GWL_EXSTYLE, g_origExStyle);

    // Notify system about frame change
    SetWindowPos(g_hWnd, nullptr, 0, 0, 0, 0, 
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    ShowWindow(g_hWnd, SW_SHOWNORMAL);
    SetForegroundWindow(g_hWnd);

    if (g_controller)
    {
        RECT rc; GetClientRect(g_hWnd, &rc);
        g_controller->put_Bounds(rc);
        g_controller->put_IsVisible(TRUE);
        g_controller->NotifyParentWindowPositionChanged();
    }
}

void HideMainWindow()
{
    ShowWindow(g_hWnd, SW_HIDE);
    // Setting IsVisible(FALSE) saves resources when hidden
    if (g_controller)
        g_controller->put_IsVisible(FALSE);
}