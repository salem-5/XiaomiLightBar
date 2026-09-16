#include "pch.h"
#include "SettingsView.h"
#include "Paths.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;

namespace LightBar {

namespace {

TextBlock MakeTitle(hstring const& text) {
    TextBlock block;
    block.Text(text);
    block.FontSize(13);
    block.FontWeight(winrt::Windows::UI::Text::FontWeights::SemiBold());
    block.Foreground(ThemeBrush(L"TextFillColorSecondaryBrush", Rgb(150, 150, 150)));
    block.Margin({ 0, 0, 0, 8 });
    return block;
}

TextBlock MakeDescription(hstring const& text) {
    TextBlock block;
    block.Text(text);
    block.FontSize(12);
    block.TextWrapping(TextWrapping::Wrap);
    block.Foreground(ThemeBrush(L"TextFillColorTertiaryBrush", Rgb(120, 120, 120)));
    block.Margin({ 0, 4, 0, 0 });
    return block;
}

std::wstring HotkeyName(UINT modifiers, UINT key) {
    std::wstring name;
    auto append = [&name](std::wstring const& part) {
        if (!name.empty()) name += L" + ";
        name += part;
    };
    if (modifiers & MOD_CONTROL) append(L"Ctrl");
    if (modifiers & MOD_ALT) append(L"Alt");
    if (modifiers & MOD_SHIFT) append(L"Shift");
    if (modifiers & MOD_WIN) append(L"Win");

    std::wstring keyName;
    if (key >= 'A' && key <= 'Z') {
        keyName.assign(1, static_cast<wchar_t>(key));
    } else if (key >= '0' && key <= '9') {
        keyName.assign(1, static_cast<wchar_t>(key));
    } else if (key >= VK_F1 && key <= VK_F24) {
        keyName = L"F" + std::to_wstring(key - VK_F1 + 1);
    } else if (key == VK_SPACE) {
        keyName = L"Space";
    } else if (key == VK_TAB) {
        keyName = L"Tab";
    } else if (key == VK_RETURN) {
        keyName = L"Enter";
    } else if (key == VK_ESCAPE) {
        keyName = L"Esc";
    } else {
        wchar_t buffer[64]{};
        const UINT scan = ::MapVirtualKeyW(key, MAPVK_VK_TO_VSC);
        if (::GetKeyNameTextW(static_cast<LONG>(scan << 16), buffer, 64) > 0) {
            keyName = buffer;
        } else {
            keyName = L"Key " + std::to_wstring(key);
        }
    }
    append(keyName);
    return name.empty() ? L"(none)" : name;
}

Border MakeCard(hstring const& title, UIElement const& content) {
    auto stack = StackPanel();
    stack.Children().Append(MakeTitle(title));
    stack.Children().Append(content);

    auto card = Border();
    card.Background(ThemeBrush(L"CardBackgroundFillColorDefaultBrush", Rgb(42, 42, 42)));
    card.BorderBrush(ThemeBrush(L"CardStrokeColorDefaultBrush", Rgb(60, 60, 60)));
    card.BorderThickness({ 1, 1, 1, 1 });
    card.CornerRadius({ 8, 8, 8, 8 });
    card.Padding({ 16, 14, 16, 16 });
    card.Child(stack);
    return card;
}

}

SettingsView::SettingsView(std::shared_ptr<Settings> settings, std::shared_ptr<SerialLink> serial)
    : settings_(std::move(settings)), serial_(std::move(serial)) {
    queue_ = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();

    remoteBox_ = TextBox();
    remoteBox_.Width(140);
    remoteBox_.MaxLength(6);
    remoteBox_.CharacterCasing(CharacterCasing::Upper);
    remoteBox_.Text(hstring(settings_->remoteId));

    auto applyButton = Button();
    applyButton.Content(box_value(L"Apply"));

    auto fetchButton = Button();
    fetchButton.Content(box_value(L"Fetch..."));

    remoteStatus_ = MakeDescription(L"");

    auto remoteRow = StackPanel();
    remoteRow.Orientation(Orientation::Horizontal);
    remoteRow.Spacing(10);
    remoteRow.VerticalAlignment(VerticalAlignment::Center);
    remoteRow.Children().Append(remoteBox_);
    remoteRow.Children().Append(applyButton);
    remoteRow.Children().Append(fetchButton);

    auto remoteStack = StackPanel();
    remoteStack.Children().Append(remoteRow);
    remoteStack.Children().Append(remoteStatus_);
    remoteStack.Children().Append(MakeDescription(
        L"The 6-digit hex ID of the remote. Set this if you re-paired the bar with a different id, "
        L"or use Fetch to capture it from the original remote."));

    auto remoteCard = MakeCard(L"Remote ID (secret key)", remoteStack);

    portCombo_ = ComboBox();
    portCombo_.IsEditable(true);
    portCombo_.MinWidth(160);
    RefreshPorts();

    auto refreshPortsButton = Button();
    refreshPortsButton.Content(box_value(L"Refresh"));

    auto portRow = StackPanel();
    portRow.Orientation(Orientation::Horizontal);
    portRow.Spacing(10);
    portRow.VerticalAlignment(VerticalAlignment::Center);
    portRow.Children().Append(portCombo_);
    portRow.Children().Append(refreshPortsButton);

    auto portStack = StackPanel();
    portStack.Children().Append(portRow);
    portStack.Children().Append(MakeDescription(L"The serial port the ESP32 is connected to."));

    auto portCard = MakeCard(L"Serial port", portStack);

    refreshPortsButton.Click([this](auto&&, auto&&) { RefreshPorts(); });

    hotkeyToggle_ = ToggleSwitch();
    hotkeyToggle_.Header(box_value(L"Enable global hotkey"));
    hotkeyToggle_.IsOn(settings_->hotkeyEnabled);

    recorderButton_ = Button();
    recorderButton_.MinWidth(200);
    recorderButton_.HorizontalAlignment(HorizontalAlignment::Left);
    recorderButton_.Margin({ 0, 10, 0, 0 });

    auto hotkeyStack = StackPanel();
    hotkeyStack.Children().Append(hotkeyToggle_);
    hotkeyStack.Children().Append(recorderButton_);
    hotkeyStack.Children().Append(
        MakeDescription(L"Click the button, then press the key combination you want."));

    auto hotkeyCard = MakeCard(L"Hotkey", hotkeyStack);

    trayToggle_ = ToggleSwitch();
    trayToggle_.Header(box_value(L"Close to tray"));
    trayToggle_.IsOn(settings_->closeToTray);

    minimizedToggle_ = ToggleSwitch();
    minimizedToggle_.Header(box_value(L"Start minimized to tray"));
    minimizedToggle_.IsOn(settings_->startMinimized);

    startupToggle_ = ToggleSwitch();
    startupToggle_.Header(box_value(L"Launch at startup"));
    startupToggle_.IsOn(settings_->launchAtStartup);

    sleepToggle_ = ToggleSwitch();
    sleepToggle_.Header(box_value(L"Turn off while the PC sleeps"));
    sleepToggle_.IsOn(settings_->sleepToggle);

    auto windowStack = StackPanel();
    windowStack.Children().Append(trayToggle_);
    windowStack.Children().Append(minimizedToggle_);
    windowStack.Children().Append(startupToggle_);
    windowStack.Children().Append(sleepToggle_);

    auto windowCard = MakeCard(L"Window", windowStack);

    stateRadio_ = RadioButtons();
    stateRadio_.Items().Append(box_value(L"On"));
    stateRadio_.Items().Append(box_value(L"Off"));
    stateRadio_.SelectedIndex(settings_->lightOn ? 0 : 1);

    auto stateStack = StackPanel();
    stateStack.Children().Append(stateRadio_);
    stateStack.Children().Append(MakeDescription(
        L"The light never reports its state, so the app tracks power by assumption. If the physical "
        L"remote drifts it, set the real state here. This does not send anything to the bar."));

    auto stateCard = MakeCard(L"Light state (sync)", stateStack);

    themeCombo_ = ComboBox();
    themeCombo_.MinWidth(200);
    for (auto const* label : { L"System", L"Light", L"Dark" }) {
        auto item = ComboBoxItem();
        item.Content(box_value(label));
        themeCombo_.Items().Append(item);
    }
    themeCombo_.SelectedIndex(settings_->theme);

    auto themeStack = StackPanel();
    themeStack.Children().Append(themeCombo_);

    auto themeCard = MakeCard(L"Theme", themeStack);

    auto content = StackPanel();
    content.Spacing(16);
    content.Margin({ 24, 20, 24, 24 });
    content.Children().Append(remoteCard);
    content.Children().Append(portCard);
    content.Children().Append(hotkeyCard);
    content.Children().Append(windowCard);
    content.Children().Append(stateCard);
    content.Children().Append(themeCard);

    root_ = ScrollViewer();
    root_.Content(content);
    root_.VerticalScrollBarVisibility(ScrollBarVisibility::Auto);

    applyButton.Click([this](auto&&, auto&&) { ApplyRemoteId(); });
    fetchButton.Click([this](auto&&, auto&&) { ShowFetchDialog(); });
}

void SettingsView::Initialize() {
    hotkeyToggle_.Toggled([this](auto&&, auto&&) {
        settings_->hotkeyEnabled = hotkeyToggle_.IsOn();
        settings_->Save();
        NotifyChanged();
    });

    recorderButton_.Click([this](auto&&, auto&&) { StartCapture(); });
    root_.PreviewKeyDown([this](
                             winrt::Windows::Foundation::IInspectable const&,
                             winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& args) {
        if (capturing_ && CaptureKey(args)) {
            args.Handled(true);
        }
    });

    portCombo_.SelectionChanged([this](auto&&, auto&&) { CommitPort(); });
    portCombo_.TextSubmitted([this](auto&&, auto&&) { CommitPort(); });
    portCombo_.LostFocus([this](auto&&, auto&&) { CommitPort(); });

    UpdateHotkeyUi();

    trayToggle_.Toggled([this](auto&&, auto&&) {
        settings_->closeToTray = trayToggle_.IsOn();
        settings_->Save();
        NotifyChanged();
    });

    minimizedToggle_.Toggled([this](auto&&, auto&&) {
        settings_->startMinimized = minimizedToggle_.IsOn();
        settings_->Save();
        NotifyChanged();
    });

    startupToggle_.Toggled([this](auto&&, auto&&) {
        settings_->launchAtStartup = startupToggle_.IsOn();
        settings_->Save();
        NotifyChanged();
    });

    sleepToggle_.Toggled([this](auto&&, auto&&) {
        settings_->sleepToggle = sleepToggle_.IsOn();
        settings_->Save();
        NotifyChanged();
    });

    stateRadio_.SelectionChanged([this](auto&&, auto&&) {
        if (stateRadio_.SelectedIndex() < 0) return;
        settings_->lightOn = stateRadio_.SelectedIndex() == 0;
        settings_->Save();
        NotifyChanged();
    });

    themeCombo_.SelectionChanged([this](auto&&, auto&&) {
        if (themeCombo_.SelectedIndex() < 0) return;
        settings_->theme = themeCombo_.SelectedIndex();
        settings_->Save();
        NotifyChanged();
    });
}

void SettingsView::ApplyRemoteId() {
    std::wstring value(remoteBox_.Text().c_str());
    if (value.size() != 6 ||
        value.find_first_not_of(L"0123456789ABCDEFabcdef") != std::wstring::npos) {
        remoteStatus_.Text(L"Enter exactly 6 hex digits (e.g. DD9863).");
        remoteStatus_.Foreground(SolidColorBrush(Rgb(230, 120, 120)));
        return;
    }

    std::transform(value.begin(), value.end(), value.begin(),
                   [](wchar_t c) { return static_cast<wchar_t>(::towupper(c)); });
    settings_->remoteId = value;
    settings_->Save();
    remoteStatus_.Text(L"Saved.");
    remoteStatus_.Foreground(SolidColorBrush(Rgb(46, 204, 113)));
    NotifyChanged();
}

void SettingsView::ShowFetchDialog() {
    if (!root_.XamlRoot()) return;

    auto instructions = TextBlock();
    instructions.Text(
        L"1. Hold your original remote right next to the nRF24L01.\n"
        L"2. Click Start, then turn or press the remote's knob a few times.\n"
        L"3. The ID is filled in automatically once a packet is captured.");
    instructions.TextWrapping(TextWrapping::Wrap);

    scanRing_ = ProgressRing();
    scanRing_.IsActive(false);
    scanRing_.IsIndeterminate(true);
    scanRing_.Width(28);
    scanRing_.Height(28);
    scanRing_.Margin({ 0, 14, 0, 0 });

    scanStatus_ = TextBlock();
    scanStatus_.Text(L"Press Start when the remote is in place.");
    scanStatus_.TextWrapping(TextWrapping::Wrap);
    scanStatus_.Margin({ 0, 12, 0, 0 });

    auto content = StackPanel();
    content.MinWidth(380);
    content.Children().Append(instructions);
    content.Children().Append(scanRing_);
    content.Children().Append(scanStatus_);

    scanDialog_ = ContentDialog();
    scanDialog_.XamlRoot(root_.XamlRoot());
    scanDialog_.Title(box_value(L"Fetch remote ID"));
    scanDialog_.PrimaryButtonText(L"Start");
    scanDialog_.CloseButtonText(L"Cancel");
    scanDialog_.DefaultButton(ContentDialogButton::Primary);
    scanDialog_.Content(content);

    scanDialog_.PrimaryButtonClick(
        [this](ContentDialog const&, ContentDialogButtonClickEventArgs const& args) {
            args.Cancel(true);
            StartScan();
        });
    scanDialog_.Closed([this](ContentDialog const&, ContentDialogClosedEventArgs const&) {
        StopScan();
    });

    scanDialog_.ShowAsync();
}

void SettingsView::StartScan() {
    if (!serial_) return;

    scanStatus_.Text(L"Listening... turn or press the knob on your remote.");
    scanStatus_.Foreground(ThemeBrush(L"TextFillColorSecondaryBrush", Rgb(150, 150, 150)));
    scanRing_.IsActive(true);

    auto queue = queue_;
    scanToken_ = serial_->AddLineHandler([this, queue](std::wstring const& line) {
        if (line.rfind(L"SCAN", 0) != 0) return;
        queue.TryEnqueue([this, line]() { HandleScanLine(line); });
    });
    serial_->Send(L"SCAN");
}

void SettingsView::StopScan() {
    if (scanToken_ && serial_) {
        serial_->RemoveLineHandler(scanToken_);
        scanToken_ = 0;
    }
    if (scanRing_) scanRing_.IsActive(false);
}

void SettingsView::HandleScanLine(std::wstring const& line) {
    if (line.rfind(L"SCAN id=", 0) == 0) {
        std::wstring id = line.substr(8);
        while (!id.empty() && (id.back() == L'\r' || id.back() == L'\n' || id.back() == L' ')) {
            id.pop_back();
        }
        if (id.size() != 6) return;

        remoteBox_.Text(hstring(id));
        settings_->remoteId = id;
        settings_->Save();
        remoteStatus_.Text(L"Fetched from the remote.");
        remoteStatus_.Foreground(SolidColorBrush(Rgb(46, 204, 113)));
        NotifyChanged();
        if (scanDialog_) scanDialog_.Hide();
    } else if (line == L"SCAN NONE") {
        scanStatus_.Text(L"No remote detected. Keep the remote close and try again.");
        scanStatus_.Foreground(SolidColorBrush(Rgb(230, 160, 40)));
        scanRing_.IsActive(false);
        StopScan();
    }
}

void SettingsView::RefreshPorts() {
    updatingPort_ = true;
    portCombo_.Items().Clear();
    for (auto const& name : EnumerateComPorts()) {
        auto item = ComboBoxItem();
        item.Content(box_value(hstring(name)));
        portCombo_.Items().Append(item);
    }
    portCombo_.Text(hstring(settings_->port));
    updatingPort_ = false;
}

void SettingsView::CommitPort() {
    if (updatingPort_) return;
    std::wstring value(portCombo_.Text().c_str());
    while (!value.empty() && value.back() == L' ') value.pop_back();
    if (value.empty() || value == settings_->port) return;
    settings_->port = value;
    settings_->Save();
    NotifyChanged();
}

void SettingsView::UpdateHotkeyUi() {
    recorderButton_.Content(
        box_value(hstring(HotkeyName(settings_->hotkeyModifiers, settings_->hotkeyKey))));
}

void SettingsView::StartCapture() {
    capturing_ = true;
    recorderButton_.Content(box_value(L"Press a key combination..."));
    recorderButton_.Focus(FocusState::Programmatic);
}

bool SettingsView::CaptureKey(winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& args) {
    if (!capturing_) return false;

    const UINT key = static_cast<UINT>(args.OriginalKey());
    switch (key) {
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL:
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU:
    case VK_LWIN:
    case VK_RWIN:
        return true;
    default:
        break;
    }

    UINT modifiers = 0;
    if (::GetKeyState(VK_CONTROL) & 0x8000) modifiers |= MOD_CONTROL;
    if (::GetKeyState(VK_MENU) & 0x8000) modifiers |= MOD_ALT;
    if (::GetKeyState(VK_SHIFT) & 0x8000) modifiers |= MOD_SHIFT;
    if ((::GetKeyState(VK_LWIN) & 0x8000) || (::GetKeyState(VK_RWIN) & 0x8000)) {
        modifiers |= MOD_WIN;
    }

    settings_->hotkeyModifiers = modifiers;
    settings_->hotkeyKey = key;
    settings_->Save();
    capturing_ = false;
    UpdateHotkeyUi();
    NotifyChanged();
    return true;
}

void SettingsView::NotifyChanged() {
    if (onChanged_) onChanged_();
}

}
