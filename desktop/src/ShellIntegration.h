#pragma once

#include "pch.h"
#include <shellapi.h>

namespace LightBar {

class ShellIntegration {
public:
    using Callback = std::function<void()>;

    ShellIntegration() = default;
    ~ShellIntegration();

    ShellIntegration(ShellIntegration const&) = delete;
    ShellIntegration& operator=(ShellIntegration const&) = delete;

    bool Create(HINSTANCE instance);

    void ShowTrayIcon(std::wstring const& tooltip);
    void HideTrayIcon();

    bool RegisterHotkey(UINT modifiers, UINT key);
    void UnregisterHotkey();

    void SetOnToggle(Callback cb) { onToggle_ = std::move(cb); }
    void SetOnShow(Callback cb) { onShow_ = std::move(cb); }
    void SetOnExit(Callback cb) { onExit_ = std::move(cb); }
    void SetOnSleep(Callback cb) { onSleep_ = std::move(cb); }
    void SetOnResume(Callback cb) { onResume_ = std::move(cb); }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    void ShowContextMenu();

    HWND hwnd_{ nullptr };
    HINSTANCE instance_{ nullptr };
    HPOWERNOTIFY displayNotify_{ nullptr };
    NOTIFYICONDATAW iconData_{};
    bool trayVisible_{ false };
    bool hotkeyRegistered_{ false };

    Callback onToggle_;
    Callback onShow_;
    Callback onExit_;
    Callback onSleep_;
    Callback onResume_;
};

}
