#include "pch.h"
#include "Settings.h"

namespace LightBar {

namespace {

constexpr wchar_t kRegPath[] = L"Software\\XiaomiLightBar";

DWORD ReadDword(HKEY key, wchar_t const* name, DWORD fallback) {
    DWORD value = 0;
    DWORD size = sizeof(value);
    DWORD type = 0;
    if (::RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<BYTE*>(&value), &size) ==
            ERROR_SUCCESS &&
        type == REG_DWORD) {
        return value;
    }
    return fallback;
}

std::wstring ReadString(HKEY key, wchar_t const* name, std::wstring const& fallback) {
    wchar_t buffer[128]{};
    DWORD size = sizeof(buffer);
    DWORD type = 0;
    if (::RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<BYTE*>(buffer), &size) ==
            ERROR_SUCCESS &&
        type == REG_SZ) {
        return std::wstring(buffer);
    }
    return fallback;
}

}

Settings::Settings() {
    Load();
}

std::vector<HotkeyPreset> const& Settings::Hotkeys() {
    static const std::vector<HotkeyPreset> presets = {
        { L"Ctrl + Alt + L", MOD_CONTROL | MOD_ALT, 'L' },
        { L"Ctrl + Shift + L", MOD_CONTROL | MOD_SHIFT, 'L' },
        { L"Ctrl + Alt + Space", MOD_CONTROL | MOD_ALT, VK_SPACE },
        { L"F8", 0, VK_F8 },
        { L"Ctrl + F8", MOD_CONTROL, VK_F8 },
    };
    return presets;
}

void Settings::Load() {
    HKEY key = nullptr;
    if (::RegOpenKeyExW(HKEY_CURRENT_USER, kRegPath, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return;
    }

    remoteId = ReadString(key, L"RemoteId", remoteId);
    port = ReadString(key, L"Port", port);
    brightness = static_cast<int>(ReadDword(key, L"Brightness", 8));
    colorTemp = static_cast<int>(ReadDword(key, L"ColorTemp", 8));
    hotkeyEnabled = ReadDword(key, L"HotkeyEnabled", 1) != 0;
    hotkeyModifiers = ReadDword(key, L"HotkeyModifiers", hotkeyModifiers);
    hotkeyKey = ReadDword(key, L"HotkeyKey", hotkeyKey);
    closeToTray = ReadDword(key, L"CloseToTray", 0) != 0;
    startMinimized = ReadDword(key, L"StartMinimized", 0) != 0;
    launchAtStartup = ReadDword(key, L"LaunchAtStartup", 0) != 0;
    sleepToggle = ReadDword(key, L"SleepToggle", 1) != 0;
    lightOn = ReadDword(key, L"LightOn", 1) != 0;
    theme = static_cast<int>(ReadDword(key, L"Theme", 0));

    ::RegCloseKey(key);

    if (remoteId.size() != 6) {
        remoteId = L"DD9863";
    }
    if (port.empty()) {
        port = L"COM3";
    }
    if (brightness < 0 || brightness > 15) brightness = 8;
    if (colorTemp < 0 || colorTemp > 15) colorTemp = 8;
}

void Settings::Save() const {
    HKEY key = nullptr;
    if (::RegCreateKeyExW(HKEY_CURRENT_USER, kRegPath, 0, nullptr, 0, KEY_WRITE, nullptr, &key,
                          nullptr) != ERROR_SUCCESS) {
        return;
    }

    auto writeString = [key](wchar_t const* name, std::wstring const& value) {
        ::RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<BYTE const*>(value.c_str()),
                         static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
    };
    auto writeDword = [key](wchar_t const* name, DWORD value) {
        ::RegSetValueExW(key, name, 0, REG_DWORD, reinterpret_cast<BYTE const*>(&value),
                         sizeof(value));
    };

    writeString(L"RemoteId", remoteId);
    writeString(L"Port", port);
    writeDword(L"Brightness", static_cast<DWORD>(brightness));
    writeDword(L"ColorTemp", static_cast<DWORD>(colorTemp));
    writeDword(L"HotkeyEnabled", hotkeyEnabled ? 1 : 0);
    writeDword(L"HotkeyModifiers", hotkeyModifiers);
    writeDword(L"HotkeyKey", hotkeyKey);
    writeDword(L"CloseToTray", closeToTray ? 1 : 0);
    writeDword(L"StartMinimized", startMinimized ? 1 : 0);
    writeDword(L"LaunchAtStartup", launchAtStartup ? 1 : 0);
    writeDword(L"SleepToggle", sleepToggle ? 1 : 0);
    writeDword(L"LightOn", lightOn ? 1 : 0);
    writeDword(L"Theme", static_cast<DWORD>(theme));

    ::RegCloseKey(key);
}

}
