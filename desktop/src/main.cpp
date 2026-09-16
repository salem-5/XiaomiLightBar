#include "pch.h"
#include "App.h"

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    winrt::Microsoft::UI::Xaml::Application::Start(
        [](winrt::Microsoft::UI::Xaml::ApplicationInitializationCallbackParams const&) {
            winrt::make<LightBar::App>();
        });

    return 0;
}
