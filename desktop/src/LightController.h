#pragma once

#include "pch.h"
#include "SerialLink.h"
#include "Settings.h"

namespace LightBar {

class LightController {
public:
    LightController(std::shared_ptr<SerialLink> serial, std::shared_ptr<Settings> settings)
        : serial_(std::move(serial)), settings_(std::move(settings)) {
        if (settings_) on_ = settings_->lightOn;
    }

    void Toggle() {
        if (serial_) serial_->Send(L"ONOFF");
        on_ = !on_;
        if (settings_) {
            settings_->lightOn = on_;
            settings_->Save();
        }
    }

    void SetPower(bool on) {
        if (on != on_) Toggle();
    }

    void SyncState(bool on) { on_ = on; }

    bool IsOn() const { return on_; }

private:
    std::shared_ptr<SerialLink> serial_;
    std::shared_ptr<Settings> settings_;
    bool on_{ true };
};

}
