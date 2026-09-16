#pragma once

#include "pch.h"

namespace LightBar {

inline std::wstring ModuleDirectory() {
    wchar_t buffer[MAX_PATH]{};
    ::GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    std::wstring path(buffer);
    const auto pos = path.find_last_of(L"\\/");
    return pos == std::wstring::npos ? path : path.substr(0, pos);
}

inline std::wstring ModulePath() {
    wchar_t buffer[MAX_PATH]{};
    ::GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return std::wstring(buffer);
}

inline std::wstring AssetPath(std::wstring const& fileName) {
    return ModuleDirectory() + L"\\" + fileName;
}

inline winrt::Windows::Foundation::Uri FileUri(std::wstring const& fullPath) {
    std::wstring path = fullPath;
    std::replace(path.begin(), path.end(), L'\\', L'/');
    return winrt::Windows::Foundation::Uri(L"file:///" + path);
}

inline winrt::Windows::UI::Color Rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return winrt::Windows::UI::ColorHelper::FromArgb(a, r, g, b);
}

inline winrt::Microsoft::UI::Xaml::Media::Brush ThemeBrush(
    wchar_t const* key, winrt::Windows::UI::Color fallback) {
    try {
        auto resources = winrt::Microsoft::UI::Xaml::Application::Current().Resources();
        auto value = resources.Lookup(winrt::box_value(key));
        if (auto brush = value.try_as<winrt::Microsoft::UI::Xaml::Media::Brush>()) return brush;
    } catch (...) {
    }
    return winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(fallback);
}

}
