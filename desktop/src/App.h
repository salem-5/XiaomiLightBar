#pragma once

#include "pch.h"
#include <winrt/Windows.UI.Xaml.Interop.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <winrt/Microsoft.UI.Xaml.XamlTypeInfo.h>

namespace LightBar {

struct App : winrt::Microsoft::UI::Xaml::ApplicationT<
                 App,
                 winrt::Microsoft::UI::Xaml::Markup::IXamlMetadataProvider> {
    App() = default;

    void OnLaunched(winrt::Microsoft::UI::Xaml::LaunchActivatedEventArgs const& args);

    winrt::Microsoft::UI::Xaml::Markup::IXamlType GetXamlType(
        winrt::Windows::UI::Xaml::Interop::TypeName const& type);
    winrt::Microsoft::UI::Xaml::Markup::IXamlType GetXamlType(winrt::hstring const& fullname);
    winrt::com_array<winrt::Microsoft::UI::Xaml::Markup::XmlnsDefinition> GetXmlnsDefinitions();

private:
    winrt::Microsoft::UI::Xaml::XamlTypeInfo::XamlControlsXamlMetaDataProvider provider_;
    std::shared_ptr<class MainWindow> window_;
};

}
