#pragma once

#include "pch.h"
#include "SerialLink.h"
#include "Settings.h"
#include "LightController.h"

namespace LightBar {

class LightBarView : public std::enable_shared_from_this<LightBarView> {
public:
    LightBarView(std::shared_ptr<SerialLink> serial, std::shared_ptr<Settings> settings,
                 std::shared_ptr<LightController> controller);

    winrt::Microsoft::UI::Xaml::FrameworkElement Root() const { return root_; }

    void Initialize();

    void SetConnectionStatus(bool connected, std::wstring const& info);

private:
    void BuildUi();
    void UpdateBrightnessVisual();
    void UpdateTempVisual();
    void SendCommand(std::wstring const& command);

    std::shared_ptr<SerialLink> serial_;
    std::shared_ptr<Settings> settings_;
    std::shared_ptr<LightController> controller_;
    winrt::Microsoft::UI::Dispatching::DispatcherQueue queue_{ nullptr };

    winrt::Microsoft::UI::Xaml::Controls::Grid root_{ nullptr };
    winrt::Microsoft::UI::Xaml::Shapes::Ellipse statusDot_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::TextBlock statusText_{ nullptr };

    winrt::Microsoft::UI::Xaml::Controls::Button onOffButton_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::Button resetButton_{ nullptr };

    winrt::Microsoft::UI::Xaml::Controls::Grid brightBar_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::Border brightFill_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::Slider brightSlider_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::TextBlock brightValue_{ nullptr };
    int bright_{ 8 };

    winrt::Microsoft::UI::Xaml::Controls::Grid tempBar_{ nullptr };
    winrt::Microsoft::UI::Xaml::Shapes::Ellipse tempMarker_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::Slider tempSlider_{ nullptr };
    winrt::Microsoft::UI::Xaml::Controls::TextBlock tempValue_{ nullptr };
    int temp_{ 8 };

    winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer brightTimer_{ nullptr };
    winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer tempTimer_{ nullptr };
};

}
