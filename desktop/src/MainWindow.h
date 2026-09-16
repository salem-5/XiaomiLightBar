#pragma once

#include "pch.h"
#include "SerialLink.h"
#include "Settings.h"
#include "LightController.h"
#include "HomeView.h"
#include "LightBarView.h"
#include "SettingsView.h"
#include "ShellIntegration.h"

namespace LightBar {

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    void Activate();

private:
    void ApplySettings();
    void ShowWindow();
    void ExitApp();

    winrt::Microsoft::UI::Xaml::Window window_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::Grid root_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::NavigationView nav_{ nullptr };

    std::shared_ptr<Settings> settings_;
    std::shared_ptr<SerialLink> serial_;
    std::shared_ptr<LightController> light_;
    std::shared_ptr<HomeView> home_;
    std::shared_ptr<LightBarView> lightBar_;
    std::shared_ptr<SettingsView> settingsView_;
    std::unique_ptr<ShellIntegration> shell_;

    bool connected_{ false };
    bool exiting_{ false };
    bool sleptOff_{ false };
};

}
