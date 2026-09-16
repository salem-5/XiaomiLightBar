#include "pch.h"
#include "HomeView.h"
#include "Paths.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;

namespace LightBar {

HomeView::HomeView(std::shared_ptr<SerialLink> serial, std::shared_ptr<LightController> controller)
    : serial_(std::move(serial)), controller_(std::move(controller)) {
    root_ = Grid();

    auto icon = Image();
    icon.Width(84);
    icon.Height(84);
    icon.Source(winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage(
        FileUri(AssetPath(L"app.png"))));

    auto title = TextBlock();
    title.Text(L"Xiaomi Light Bar");
    title.FontSize(26);
    title.FontWeight(winrt::Windows::UI::Text::FontWeights::SemiBold());
    title.HorizontalAlignment(HorizontalAlignment::Center);
    title.Margin({ 0, 16, 0, 0 });

    auto subtitle = TextBlock();
    subtitle.Text(L"Monitor light bar controller");
    subtitle.FontSize(13);
    subtitle.Foreground(ThemeBrush(L"TextFillColorSecondaryBrush", Rgb(150, 150, 150)));
    subtitle.HorizontalAlignment(HorizontalAlignment::Center);
    subtitle.Margin({ 0, 4, 0, 0 });

    statusDot_ = winrt::Microsoft::UI::Xaml::Shapes::Ellipse();
    statusDot_.Width(10);
    statusDot_.Height(10);
    statusDot_.Fill(SolidColorBrush(Rgb(230, 160, 40)));
    statusDot_.VerticalAlignment(VerticalAlignment::Center);

    statusText_ = TextBlock();
    statusText_.Text(L"Searching for COM3...");
    statusText_.FontSize(14);
    statusText_.VerticalAlignment(VerticalAlignment::Center);

    auto statusRow = StackPanel();
    statusRow.Orientation(Orientation::Horizontal);
    statusRow.Spacing(10);
    statusRow.HorizontalAlignment(HorizontalAlignment::Center);
    statusRow.Children().Append(statusDot_);
    statusRow.Children().Append(statusText_);

    auto statusCard = Border();
    statusCard.Background(ThemeBrush(L"CardBackgroundFillColorDefaultBrush", Rgb(42, 42, 42)));
    statusCard.BorderBrush(ThemeBrush(L"CardStrokeColorDefaultBrush", Rgb(60, 60, 60)));
    statusCard.BorderThickness({ 1, 1, 1, 1 });
    statusCard.CornerRadius({ 8, 8, 8, 8 });
    statusCard.Padding({ 16, 14, 16, 14 });
    statusCard.Margin({ 0, 28, 0, 0 });
    statusCard.Child(statusRow);

    onOffButton_ = Button();
    onOffButton_.Content(box_value(L"On / Off"));
    onOffButton_.MinWidth(150);

    resetButton_ = Button();
    resetButton_.Content(box_value(L"Reset"));
    resetButton_.MinWidth(150);

    auto actions = StackPanel();
    actions.Orientation(Orientation::Horizontal);
    actions.Spacing(12);
    actions.HorizontalAlignment(HorizontalAlignment::Center);
    actions.Margin({ 0, 24, 0, 0 });
    actions.Children().Append(onOffButton_);
    actions.Children().Append(resetButton_);

    auto hint = TextBlock();
    hint.Text(L"Brightness and color temperature are on the Light Bar page.");
    hint.FontSize(12);
    hint.Foreground(ThemeBrush(L"TextFillColorTertiaryBrush", Rgb(120, 120, 120)));
    hint.HorizontalAlignment(HorizontalAlignment::Center);
    hint.TextWrapping(TextWrapping::Wrap);
    hint.Margin({ 0, 18, 0, 0 });

    auto stack = StackPanel();
    stack.HorizontalAlignment(HorizontalAlignment::Center);
    stack.VerticalAlignment(VerticalAlignment::Center);
    stack.MaxWidth(520);
    stack.Margin({ 32, 24, 32, 24 });
    stack.Children().Append(icon);
    stack.Children().Append(title);
    stack.Children().Append(subtitle);
    stack.Children().Append(statusCard);
    stack.Children().Append(actions);
    stack.Children().Append(hint);

    auto scroll = ScrollViewer();
    scroll.Content(stack);
    scroll.VerticalScrollBarVisibility(ScrollBarVisibility::Auto);
    root_.Children().Append(scroll);
}

void HomeView::Initialize() {
    auto weak = weak_from_this();
    onOffButton_.Click([weak](auto&&, auto&&) {
        if (auto self = weak.lock()) {
            if (self->controller_) self->controller_->Toggle();
        }
    });
    resetButton_.Click([weak](auto&&, auto&&) {
        if (auto self = weak.lock()) {
            if (self->serial_) self->serial_->Send(L"RESET");
        }
    });
}

void HomeView::SetConnectionStatus(bool connected, std::wstring const& info) {
    statusDot_.Fill(SolidColorBrush(connected ? Rgb(46, 204, 113) : Rgb(230, 160, 40)));
    statusText_.Text(hstring(info));
}

}
