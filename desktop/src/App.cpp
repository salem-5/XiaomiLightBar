#include "pch.h"
#include "App.h"
#include "MainWindow.h"

namespace LightBar {

winrt::Microsoft::UI::Xaml::Markup::IXamlType App::GetXamlType(
    winrt::Windows::UI::Xaml::Interop::TypeName const& type) {
    return provider_.GetXamlType(type);
}

winrt::Microsoft::UI::Xaml::Markup::IXamlType App::GetXamlType(winrt::hstring const& fullname) {
    return provider_.GetXamlType(fullname);
}

winrt::com_array<winrt::Microsoft::UI::Xaml::Markup::XmlnsDefinition> App::GetXmlnsDefinitions() {
    return provider_.GetXmlnsDefinitions();
}

void App::OnLaunched(winrt::Microsoft::UI::Xaml::LaunchActivatedEventArgs const&) {
    Resources().MergedDictionaries().Append(
        winrt::Microsoft::UI::Xaml::Controls::XamlControlsResources());

    window_ = std::make_shared<MainWindow>();
    window_->Activate();
}

}
