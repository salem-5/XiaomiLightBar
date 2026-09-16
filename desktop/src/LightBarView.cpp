#include "pch.h"
#include "LightBarView.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;

namespace LightBar {

namespace {

winrt::Windows::UI::Color Rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return winrt::Windows::UI::ColorHelper::FromArgb(a, r, g, b);
}

Brush ThemeBrush(wchar_t const* key, winrt::Windows::UI::Color fallback) {
    try {
        auto resources = Application::Current().Resources();
        auto value = resources.Lookup(box_value(key));
        if (auto brush = value.try_as<Brush>()) return brush;
    } catch (...) {
    }
    return SolidColorBrush(fallback);
}

ColumnDefinition AutoColumn() {
    ColumnDefinition column;
    column.Width(GridLengthHelper::Auto());
    return column;
}

ColumnDefinition StarColumn() {
    ColumnDefinition column;
    column.Width(GridLengthHelper::FromValueAndType(1.0, GridUnitType::Star));
    return column;
}

TextBlock MakeCardTitle(hstring const& text) {
    TextBlock block;
    block.Text(text);
    block.FontSize(13);
    block.FontWeight(winrt::Windows::UI::Text::FontWeights::SemiBold());
    block.Foreground(ThemeBrush(L"TextFillColorSecondaryBrush", Rgb(150, 150, 150)));
    return block;
}

Border MakeCard(hstring const& title, UIElement const& body, FrameworkElement const& trailing = nullptr) {
    auto header = Grid();
    header.ColumnDefinitions().Append(AutoColumn());
    header.ColumnDefinitions().Append(StarColumn());
    header.Children().Append(MakeCardTitle(title));
    if (trailing) {
        Grid::SetColumn(trailing, 1);
        trailing.HorizontalAlignment(HorizontalAlignment::Right);
        header.Children().Append(trailing);
    }

    auto stack = StackPanel();
    stack.Children().Append(header);
    stack.Children().Append(body);

    auto card = Border();
    card.Background(ThemeBrush(L"CardBackgroundFillColorDefaultBrush", Rgb(42, 42, 42)));
    card.BorderBrush(ThemeBrush(L"CardStrokeColorDefaultBrush", Rgb(60, 60, 60)));
    card.BorderThickness({ 1, 1, 1, 1 });
    card.CornerRadius({ 8, 8, 8, 8 });
    card.Padding({ 16, 14, 16, 16 });
    card.Child(stack);
    return card;
}

TextBlock MakeValueLabel() {
    TextBlock block;
    block.FontSize(13);
    block.Foreground(ThemeBrush(L"TextFillColorSecondaryBrush", Rgb(150, 150, 150)));
    block.VerticalAlignment(VerticalAlignment::Center);
    return block;
}

Slider MakeSlider() {
    Slider slider;
    slider.Minimum(0);
    slider.Maximum(15);
    slider.StepFrequency(1);
    slider.TickFrequency(1);
    slider.SnapsTo(winrt::Microsoft::UI::Xaml::Controls::Primitives::SliderSnapsTo::StepValues);
    slider.Margin({ 0, 14, 0, 0 });
    return slider;
}

}

LightBarView::LightBarView(std::shared_ptr<SerialLink> serial, std::shared_ptr<Settings> settings,
                           std::shared_ptr<LightController> controller)
    : serial_(std::move(serial)), settings_(std::move(settings)),
      controller_(std::move(controller)) {
    queue_ = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
    if (settings_) {
        bright_ = settings_->brightness;
        temp_ = settings_->colorTemp;
    }
    BuildUi();
}

void LightBarView::BuildUi() {

    statusDot_ = winrt::Microsoft::UI::Xaml::Shapes::Ellipse();
    statusDot_.Width(10);
    statusDot_.Height(10);
    statusDot_.Fill(SolidColorBrush(Rgb(230, 160, 40)));
    statusDot_.VerticalAlignment(VerticalAlignment::Center);

    statusText_ = TextBlock();
    statusText_.Text(L"Searching for COM3...");
    statusText_.FontSize(13);
    statusText_.VerticalAlignment(VerticalAlignment::Center);

    auto statusRow = StackPanel();
    statusRow.Orientation(Orientation::Horizontal);
    statusRow.Spacing(10);
    statusRow.VerticalAlignment(VerticalAlignment::Center);
    statusRow.Children().Append(statusDot_);
    statusRow.Children().Append(statusText_);

    auto statusBar = Border();
    statusBar.Padding({ 24, 14, 24, 14 });
    statusBar.BorderThickness({ 0, 0, 0, 1 });
    statusBar.BorderBrush(ThemeBrush(L"DividerStrokeColorDefaultBrush", Rgb(60, 60, 60)));
    statusBar.Child(statusRow);

    onOffButton_ = Button();
    onOffButton_.Content(box_value(L"On / Off"));
    onOffButton_.Width(140);
    resetButton_ = Button();
    resetButton_.Content(box_value(L"Reset"));
    resetButton_.Width(140);

    auto powerRow = StackPanel();
    powerRow.Orientation(Orientation::Horizontal);
    powerRow.Spacing(12);
    powerRow.Children().Append(onOffButton_);
    powerRow.Children().Append(resetButton_);

    auto brightTrack = Border();
    brightTrack.Background(ThemeBrush(L"ControlFillColorSecondaryBrush", Rgb(55, 55, 55)));
    brightTrack.CornerRadius({ 6, 6, 6, 6 });

    brightFill_ = Border();
    brightFill_.Background(SolidColorBrush(Rgb(255, 214, 150)));
    brightFill_.CornerRadius({ 6, 6, 6, 6 });
    brightFill_.HorizontalAlignment(HorizontalAlignment::Left);
    brightFill_.Width(0);

    brightBar_ = Grid();
    brightBar_.Height(28);
    brightBar_.Margin({ 0, 12, 0, 0 });
    brightBar_.Children().Append(brightTrack);
    brightBar_.Children().Append(brightFill_);

    brightSlider_ = MakeSlider();
    brightValue_ = MakeValueLabel();

    auto brightBody = StackPanel();
    brightBody.Children().Append(brightBar_);
    brightBody.Children().Append(brightSlider_);

    auto gradient = LinearGradientBrush();
    gradient.StartPoint({ 0.0f, 0.5f });
    gradient.EndPoint({ 1.0f, 0.5f });
    auto addStop = [&gradient](winrt::Windows::UI::Color color, double offset) {
        GradientStop stop;
        stop.Color(color);
        stop.Offset(offset);
        gradient.GradientStops().Append(stop);
    };
    addStop(Rgb(255, 168, 64), 0.0);
    addStop(Rgb(255, 244, 226), 0.5);
    addStop(Rgb(148, 194, 255), 1.0);

    auto tempTrack = Border();
    tempTrack.Background(gradient);
    tempTrack.CornerRadius({ 6, 6, 6, 6 });

    tempMarker_ = winrt::Microsoft::UI::Xaml::Shapes::Ellipse();
    tempMarker_.Width(16);
    tempMarker_.Height(16);
    tempMarker_.Fill(SolidColorBrush(Rgb(255, 255, 255)));
    tempMarker_.Stroke(SolidColorBrush(Rgb(25, 25, 25)));
    tempMarker_.StrokeThickness(2);
    tempMarker_.HorizontalAlignment(HorizontalAlignment::Left);
    tempMarker_.VerticalAlignment(VerticalAlignment::Center);

    tempBar_ = Grid();
    tempBar_.Height(28);
    tempBar_.Margin({ 0, 12, 0, 0 });
    tempBar_.Children().Append(tempTrack);
    tempBar_.Children().Append(tempMarker_);

    tempSlider_ = MakeSlider();
    tempValue_ = MakeValueLabel();

    auto tempBody = StackPanel();
    tempBody.Children().Append(tempBar_);
    tempBody.Children().Append(tempSlider_);

    auto content = StackPanel();
    content.Spacing(16);
    content.Margin({ 24, 20, 24, 24 });
    content.Children().Append(MakeCard(L"Power", powerRow));
    content.Children().Append(MakeCard(L"Brightness", brightBody, brightValue_));
    content.Children().Append(MakeCard(L"Color temperature", tempBody, tempValue_));

    auto scroll = ScrollViewer();
    scroll.Content(content);
    scroll.VerticalScrollBarVisibility(ScrollBarVisibility::Auto);

    root_ = Grid();
    {
        RowDefinition row0;
        row0.Height(GridLengthHelper::Auto());
        RowDefinition row1;
        row1.Height(GridLengthHelper::FromValueAndType(1.0, GridUnitType::Star));
        root_.RowDefinitions().Append(row0);
        root_.RowDefinitions().Append(row1);
    }
    Grid::SetRow(statusBar, 0);
    Grid::SetRow(scroll, 1);
    root_.Children().Append(statusBar);
    root_.Children().Append(scroll);

    brightSlider_.Value(bright_);
    tempSlider_.Value(temp_);
}

void LightBarView::Initialize() {
    auto weak = weak_from_this();

    onOffButton_.Click([weak](auto&&, auto&&) {
        if (auto self = weak.lock()) {
            if (self->controller_) self->controller_->Toggle();
        }
    });
    resetButton_.Click([weak](auto&&, auto&&) {
        if (auto self = weak.lock()) self->SendCommand(L"RESET");
    });

    brightSlider_.ValueChanged([weak](auto&& sender, auto&&) {
        auto self = weak.lock();
        if (!self) return;
        auto slider = sender.try_as<Slider>();
        if (!slider) return;
        self->bright_ = static_cast<int>(std::lround(slider.Value()));
        self->UpdateBrightnessVisual();
        self->brightTimer_.Start();
    });
    tempSlider_.ValueChanged([weak](auto&& sender, auto&&) {
        auto self = weak.lock();
        if (!self) return;
        auto slider = sender.try_as<Slider>();
        if (!slider) return;
        self->temp_ = static_cast<int>(std::lround(slider.Value()));
        self->UpdateTempVisual();
        self->tempTimer_.Start();
    });

    brightBar_.SizeChanged([weak](auto&&, auto&&) {
        if (auto self = weak.lock()) self->UpdateBrightnessVisual();
    });
    tempBar_.SizeChanged([weak](auto&&, auto&&) {
        if (auto self = weak.lock()) self->UpdateTempVisual();
    });

    brightTimer_ = queue_.CreateTimer();
    brightTimer_.Interval(std::chrono::milliseconds(120));
    brightTimer_.IsRepeating(false);
    brightTimer_.Tick([weak](auto&&, auto&&) {
        if (auto self = weak.lock()) {
            self->SendCommand(L"BRIGHT:" + std::to_wstring(self->bright_));
            if (self->settings_) {
                self->settings_->brightness = self->bright_;
                self->settings_->Save();
            }
        }
    });

    tempTimer_ = queue_.CreateTimer();
    tempTimer_.Interval(std::chrono::milliseconds(120));
    tempTimer_.IsRepeating(false);
    tempTimer_.Tick([weak](auto&&, auto&&) {
        if (auto self = weak.lock()) {
            self->SendCommand(L"TEMP:" + std::to_wstring(self->temp_));
            if (self->settings_) {
                self->settings_->colorTemp = self->temp_;
                self->settings_->Save();
            }
        }
    });

    UpdateBrightnessVisual();
    UpdateTempVisual();
}

void LightBarView::SetConnectionStatus(bool connected, std::wstring const& info) {
    statusText_.Text(hstring(info));
    statusDot_.Fill(SolidColorBrush(connected ? Rgb(46, 204, 113) : Rgb(230, 160, 40)));
    if (connected) {
        SendCommand(L"BRIGHT:" + std::to_wstring(bright_));
        SendCommand(L"TEMP:" + std::to_wstring(temp_));
    }
}

void LightBarView::UpdateBrightnessVisual() {
    brightValue_.Text(hstring(std::to_wstring(bright_) + L" / 15"));
    const double width = brightBar_.ActualWidth();
    brightFill_.Width(width * (bright_ / 15.0));
}

void LightBarView::UpdateTempVisual() {
    const double width = tempBar_.ActualWidth();
    const double marker = 16.0;
    const double x = std::max(0.0, width - marker) * (temp_ / 15.0);
    tempMarker_.Margin({ x, 0, 0, 0 });

    const int kelvin = 2700 + static_cast<int>(std::lround((temp_ / 15.0) * (6500 - 2700)));
    tempValue_.Text(hstring(std::to_wstring(kelvin) + L" K"));
}

void LightBarView::SendCommand(std::wstring const& command) {
    if (serial_) serial_->Send(command);
}

}
