#include "pch.h"
#include "ShellIntegration.h"

#include <shellapi.h>

namespace LightBar {

namespace {

constexpr UINT kTrayMessage = WM_APP + 1;
constexpr UINT kHotkeyId = 1;
constexpr UINT kAppIconId = 101;
constexpr UINT kMenuShow = 1;
constexpr UINT kMenuToggle = 2;
constexpr UINT kMenuExit = 3;
constexpr wchar_t kWindowClass[] = L"XiaomiLightBar.ShellIntegration";

}

ShellIntegration::~ShellIntegration() {
    UnregisterHotkey();
    HideTrayIcon();
    if (displayNotify_) {
        ::UnregisterPowerSettingNotification(displayNotify_);
        displayNotify_ = nullptr;
    }
    if (hwnd_) {
        ::DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    if (instance_) {
        ::UnregisterClassW(kWindowClass, instance_);
    }
}

bool ShellIntegration::Create(HINSTANCE instance) {
    instance_ = instance;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &ShellIntegration::WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = kWindowClass;
    if (!::RegisterClassExW(&wc)) return false;

    hwnd_ = ::CreateWindowExW(0, kWindowClass, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
                              instance, this);
    if (!hwnd_) return false;

    displayNotify_ = ::RegisterPowerSettingNotification(
        hwnd_, &GUID_CONSOLE_DISPLAY_STATE, DEVICE_NOTIFY_WINDOW_HANDLE);

    return true;
}

LRESULT CALLBACK ShellIntegration::WindowProc(HWND hwnd, UINT message, WPARAM wparam,
                                              LPARAM lparam) {
    if (message == WM_NCCREATE) {
        auto create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        auto self = reinterpret_cast<ShellIntegration*>(create->lpCreateParams);
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    auto self = reinterpret_cast<ShellIntegration*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self) return self->HandleMessage(hwnd, message, wparam, lparam);
    return ::DefWindowProcW(hwnd, message, wparam, lparam);
}

LRESULT ShellIntegration::HandleMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_HOTKEY:
        if (wparam == kHotkeyId && onToggle_) onToggle_();
        return 0;

    case WM_POWERBROADCAST:
        if (wparam == PBT_APMSUSPEND) {
            if (onSleep_) onSleep_();
        } else if (wparam == PBT_APMRESUMEAUTOMATIC || wparam == PBT_APMRESUMESUSPEND) {
            if (onResume_) onResume_();
        } else if (wparam == PBT_POWERSETTINGCHANGE && lparam) {
            auto setting = reinterpret_cast<POWERBROADCAST_SETTING const*>(lparam);
            if (setting->PowerSetting == GUID_CONSOLE_DISPLAY_STATE) {
                if (setting->Data[0] == 0) {
                    if (onSleep_) onSleep_();
                } else {
                    if (onResume_) onResume_();
                }
            }
        }
        return TRUE;

    case kTrayMessage:
        switch (LOWORD(lparam)) {
        case WM_LBUTTONDBLCLK:
            if (onShow_) onShow_();
            return 0;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowContextMenu();
            return 0;
        default:
            return 0;
        }

    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case kMenuShow:
            if (onShow_) onShow_();
            return 0;
        case kMenuToggle:
            if (onToggle_) onToggle_();
            return 0;
        case kMenuExit:
            if (onExit_) onExit_();
            return 0;
        default:
            break;
        }
        break;

    default:
        break;
    }
    return ::DefWindowProcW(hwnd, message, wparam, lparam);
}

void ShellIntegration::ShowTrayIcon(std::wstring const& tooltip) {
    iconData_ = {};
    iconData_.cbSize = sizeof(iconData_);
    iconData_.hWnd = hwnd_;
    iconData_.uID = 1;
    iconData_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    iconData_.uCallbackMessage = kTrayMessage;
    iconData_.hIcon = ::LoadIconW(instance_, MAKEINTRESOURCEW(kAppIconId));
    wcsncpy_s(iconData_.szTip, tooltip.c_str(), _TRUNCATE);

    if (::Shell_NotifyIconW(NIM_ADD, &iconData_)) {
        trayVisible_ = true;
    }
}

void ShellIntegration::HideTrayIcon() {
    if (trayVisible_) {
        ::Shell_NotifyIconW(NIM_DELETE, &iconData_);
        trayVisible_ = false;
    }
}

void ShellIntegration::ShowContextMenu() {
    HMENU menu = ::CreatePopupMenu();
    if (!menu) return;

    ::AppendMenuW(menu, MF_STRING, kMenuShow, L"Show");
    ::AppendMenuW(menu, MF_STRING, kMenuToggle, L"Toggle On / Off");
    ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(menu, MF_STRING, kMenuExit, L"Exit");

    POINT point{};
    ::GetCursorPos(&point);
    ::SetForegroundWindow(hwnd_);
    ::TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, point.x, point.y, 0, hwnd_, nullptr);
    ::DestroyMenu(menu);
}

bool ShellIntegration::RegisterHotkey(UINT modifiers, UINT key) {
    UnregisterHotkey();
    if (!hwnd_) return false;
    if (::RegisterHotKey(hwnd_, kHotkeyId, modifiers, key)) {
        hotkeyRegistered_ = true;
        return true;
    }
    return false;
}

void ShellIntegration::UnregisterHotkey() {
    if (hotkeyRegistered_ && hwnd_) {
        ::UnregisterHotKey(hwnd_, kHotkeyId);
        hotkeyRegistered_ = false;
    }
}

}
