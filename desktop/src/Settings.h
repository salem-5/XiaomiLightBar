#pragma once

#include "pch.h"

namespace LightBar {

struct HotkeyPreset {
    wchar_t const* label;
    UINT modifiers;
    UINT key;
};

class Settings {
public:
    Settings();

    void Load();
    void Save() const;

    std::wstring remoteId{ L"DD9863" };
    std::wstring port{ L"COM3" };
    int brightness{ 8 };
    int colorTemp{ 8 };
    bool hotkeyEnabled{ true };
    UINT hotkeyModifiers{ MOD_CONTROL | MOD_ALT };
    UINT hotkeyKey{ 'L' };
    bool closeToTray{ false };
    bool startMinimized{ false };
    bool launchAtStartup{ false };
    bool sleepToggle{ true };
    bool lightOn{ true };
    int theme{ 0 };

    static std::vector<HotkeyPreset> const& Hotkeys();
};

}
