#pragma once

#include "pch.h"
#include "Settings.h"
#include "SerialLink.h"

namespace LightBar {

class SettingsView {
public:
    SettingsView(std::shared_ptr<Settings> settings, std::shared_ptr<SerialLink> serial);

    winrt::Microsoft::UI::Xaml::FrameworkElement Root() const { return root_; }

    void Initialize();
    void SetOnChanged(std::function<void()> cb) { onChanged_ = std::move(cb); }

private:
    void ApplyRemoteId();
    void ShowFetchDialog();
    void StartScan();
    void StopScan();
    void HandleScanLine(std::wstring const& line);
    void RefreshPorts();
    void CommitPort();
    void UpdateHotkeyUi();
    void StartCapture();
    bool CaptureKey(winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& args);
    void NotifyChanged();

    std::shared_ptr<Settings> settings_;
    std::shared_ptr<SerialLink> serial_;
    std::function<void()> onChanged_;

    winrt::Microsoft::UI::Dispatching::DispatcherQueue queue_{ nullptr };
    uint64_t scanToken_{ 0 };

    winrt::Microsoft::UI::Xaml::Controls::ScrollViewer root_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::TextBox remoteBox_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::TextBlock remoteStatus_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::ContentDialog scanDialog_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::TextBlock scanStatus_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::ProgressRing scanRing_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::ToggleSwitch hotkeyToggle_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::Button recorderButton_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::ComboBox portCombo_{ nullptr };
    bool updatingPort_{ false };
    bool capturing_{ false };
    winrt::Microsoft::UI::Xaml::Controls::ToggleSwitch trayToggle_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::ToggleSwitch minimizedToggle_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::ToggleSwitch startupToggle_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::ToggleSwitch sleepToggle_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::ComboBox themeCombo_{ nullptr };
};

}
