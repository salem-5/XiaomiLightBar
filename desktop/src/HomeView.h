#pragma once

#include "pch.h"
#include "SerialLink.h"
#include "LightController.h"

namespace LightBar {

class HomeView : public std::enable_shared_from_this<HomeView> {
public:
    HomeView(std::shared_ptr<SerialLink> serial, std::shared_ptr<LightController> controller);

    winrt::Microsoft::UI::Xaml::FrameworkElement Root() const { return root_; }

    void Initialize();
    void SetConnectionStatus(bool connected, std::wstring const& info);

private:
    std::shared_ptr<SerialLink> serial_;
    std::shared_ptr<LightController> controller_;
    winrt::Microsoft::UI::Xaml::Controls::Grid root_{ nullptr };
    winrt::Microsoft::UI::Xaml::Shapes::Ellipse statusDot_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::TextBlock statusText_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::Button onOffButton_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::Button resetButton_{ nullptr };
};

}
