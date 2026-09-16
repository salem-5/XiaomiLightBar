#include "pch.h"
#include "MainWindow.h"
#include "Paths.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;

namespace LightBar {

namespace {

void SetLaunchAtStartup(bool enabled) {
    HKEY key = nullptr;
    if (::RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                        0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) {
        return;
    }
    if (enabled) {
        std::wstring command = L"\"" + ModulePath() + L"\"";
        ::RegSetValueExW(key, L"XiaomiLightBar", 0, REG_SZ,
                         reinterpret_cast<BYTE const*>(command.c_str()),
                         static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    } else {
        ::RegDeleteValueW(key, L"XiaomiLightBar");
    }
    ::RegCloseKey(key);
}

}

MainWindow::MainWindow() {
    settings_ = std::make_shared<Settings>();
    serial_ = std::make_shared<SerialLink>(settings_->port);
    light_ = std::make_shared<LightController>(serial_, settings_);
    home_ = std::make_shared<HomeView>(serial_, light_);
    lightBar_ = std::make_shared<LightBarView>(serial_, settings_, light_);
    settingsView_ = std::make_shared<SettingsView>(settings_, serial_);

    window_ = Window();
    window_.Title(L"Xiaomi Light Bar");

    try {
        window_.SystemBackdrop(MicaBackdrop());
    } catch (...) {
    }

    try {
        window_.AppWindow().SetIcon(AssetPath(L"app.ico"));
    } catch (...) {
    }

    auto titleIcon = Image();
    titleIcon.Width(18);
    titleIcon.Height(18);
    titleIcon.Source(winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage(
        FileUri(AssetPath(L"app.png"))));

    auto titleText = TextBlock();
    titleText.Text(L"Xiaomi Light Bar");
    titleText.FontSize(13);
    titleText.VerticalAlignment(VerticalAlignment::Center);

    auto titleContent = StackPanel();
    titleContent.Orientation(Orientation::Horizontal);
    titleContent.Spacing(10);
    titleContent.VerticalAlignment(VerticalAlignment::Center);
    titleContent.Margin({ 16, 0, 0, 0 });
    titleContent.Children().Append(titleIcon);
    titleContent.Children().Append(titleText);

    auto titleBar = Grid();
    titleBar.Height(48);
    titleBar.Children().Append(titleContent);

    try {
        auto caption = window_.AppWindow().TitleBar();
        caption.ButtonBackgroundColor(winrt::Microsoft::UI::Colors::Transparent());
        caption.ButtonInactiveBackgroundColor(winrt::Microsoft::UI::Colors::Transparent());
    } catch (...) {
    }

    nav_ = NavigationView();
    nav_.IsSettingsVisible(true);
    nav_.PaneDisplayMode(NavigationViewPaneDisplayMode::Left);
    nav_.OpenPaneLength(220);
    nav_.IsBackButtonVisible(NavigationViewBackButtonVisible::Collapsed);
    nav_.PaneTitle(L"Xiaomi Light Bar");

    auto homeItem = NavigationViewItem();
    homeItem.Content(box_value(L"Home"));
    auto homeIcon = FontIcon();
    homeIcon.Glyph(L"\uE80F");
    homeItem.Icon(homeIcon);

    auto barItem = NavigationViewItem();
    barItem.Content(box_value(L"Light Bar"));
    auto barIcon = FontIcon();
    barIcon.Glyph(L"\uE706");
    barItem.Icon(barIcon);

    nav_.MenuItems().Append(homeItem);
    nav_.MenuItems().Append(barItem);

    auto homeRoot = home_->Root();
    auto barRoot = lightBar_->Root();
    auto settingsRoot = settingsView_->Root();
    nav_.Content(homeRoot);
    nav_.SelectedItem(homeItem);

    nav_.SelectionChanged([homeItem, barItem, homeRoot, barRoot, settingsRoot](
                              NavigationView const& sender,
                              NavigationViewSelectionChangedEventArgs const& args) {
        if (args.IsSettingsSelected()) {
            sender.Content(settingsRoot);
        } else if (args.SelectedItem() == homeItem) {
            sender.Content(homeRoot);
        } else if (args.SelectedItem() == barItem) {
            sender.Content(barRoot);
        }
    });

    root_ = Grid();
    {
        RowDefinition row0;
        row0.Height(GridLengthHelper::Auto());
        RowDefinition row1;
        row1.Height(GridLengthHelper::FromValueAndType(1.0, GridUnitType::Star));
        root_.RowDefinitions().Append(row0);
        root_.RowDefinitions().Append(row1);
    }
    Grid::SetRow(titleBar, 0);
    Grid::SetRow(nav_, 1);
    root_.Children().Append(titleBar);
    root_.Children().Append(nav_);

    window_.Content(root_);
    window_.ExtendsContentIntoTitleBar(true);
    window_.SetTitleBar(titleBar);
    window_.AppWindow().Resize({ 1000, 720 });

    home_->Initialize();
    lightBar_->Initialize();
    settingsView_->Initialize();
    settingsView_->SetOnChanged([this]() { ApplySettings(); });

    shell_ = std::make_unique<ShellIntegration>();
    shell_->Create(::GetModuleHandleW(nullptr));
    shell_->SetOnToggle([this]() {
        if (light_) light_->Toggle();
    });
    shell_->SetOnShow([this]() { ShowWindow(); });
    shell_->SetOnExit([this]() { ExitApp(); });
    shell_->SetOnSleep([this]() {
        if (settings_->sleepToggle && light_ && light_->IsOn()) {
            light_->SetPower(false);
            sleptOff_ = true;
        }
    });
    shell_->SetOnResume([this]() {
        if (sleptOff_ && light_) {
            light_->SetPower(true);
            sleptOff_ = false;
        }
    });
    shell_->ShowTrayIcon(L"Xiaomi Light Bar");

    ApplySettings();

    auto queue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
    std::weak_ptr<HomeView> weakHome = home_;
    std::weak_ptr<LightBarView> weakBar = lightBar_;
    serial_->SetStatusHandler([this, queue, weakHome, weakBar](
                                  bool connected, std::wstring const& info) {
        queue.TryEnqueue([this, weakHome, weakBar, connected, info]() {
            connected_ = connected;
            if (auto home = weakHome.lock()) home->SetConnectionStatus(connected, info);
            if (auto bar = weakBar.lock()) bar->SetConnectionStatus(connected, info);
            if (connected) {
                serial_->Send(L"ID:" + settings_->remoteId);
            }
        });
    });
    serial_->Start();

    window_.AppWindow().Closing([this](winrt::Microsoft::UI::Windowing::AppWindow const&,
                                       winrt::Microsoft::UI::Windowing::AppWindowClosingEventArgs const& args) {
        if (settings_->closeToTray && !exiting_) {
            args.Cancel(true);
            window_.AppWindow().Hide();
        }
    });
}

MainWindow::~MainWindow() {
    if (shell_) {
        shell_->HideTrayIcon();
    }
}

void MainWindow::ApplySettings() {
    ElementTheme theme = ElementTheme::Default;
    if (settings_->theme == 1) {
        theme = ElementTheme::Light;
    } else if (settings_->theme == 2) {
        theme = ElementTheme::Dark;
    }
    root_.RequestedTheme(theme);

    if (serial_) {
        serial_->SetPort(settings_->port);
    }

    if (shell_) {
        if (settings_->hotkeyEnabled) {
            shell_->RegisterHotkey(settings_->hotkeyModifiers, settings_->hotkeyKey);
        } else {
            shell_->UnregisterHotkey();
        }
    }

    if (connected_) {
        serial_->Send(L"ID:" + settings_->remoteId);
    }

    SetLaunchAtStartup(settings_->launchAtStartup);
}

void MainWindow::ShowWindow() {
    window_.AppWindow().Show();
    window_.Activate();
}

void MainWindow::ExitApp() {
    exiting_ = true;
    if (shell_) shell_->HideTrayIcon();
    winrt::Microsoft::UI::Xaml::Application::Current().Exit();
}

void MainWindow::Activate() {
    if (settings_->startMinimized) {
        window_.AppWindow().Hide();
    } else {
        window_.Activate();
    }
}

}
