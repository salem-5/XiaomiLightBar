#pragma once

#include "pch.h"

namespace LightBar {

class SerialLink {
public:
    using LineHandler = std::function<void(std::wstring const&)>;
    using StatusHandler = std::function<void(bool connected, std::wstring const& info)>;

    explicit SerialLink(std::wstring portName);
    ~SerialLink();

    SerialLink(SerialLink const&) = delete;
    SerialLink& operator=(SerialLink const&) = delete;

    void Start();
    void Stop();
    void SetPort(std::wstring const& portName);
    void Send(std::wstring const& command);

    uint64_t AddLineHandler(LineHandler handler);
    void RemoveLineHandler(uint64_t token);

    void SetStatusHandler(StatusHandler handler) { statusHandler_ = std::move(handler); }

private:
    void Worker();
    bool OpenLocked();
    void CloseLocked();

    std::wstring port_;
    HANDLE handle_{ INVALID_HANDLE_VALUE };
    std::thread worker_;
    std::atomic<bool> running_{ false };
    std::mutex mutex_;
    std::mutex handlersMutex_;
    std::vector<std::pair<uint64_t, LineHandler>> lineHandlers_;
    uint64_t nextHandlerToken_{ 1 };
    StatusHandler statusHandler_;
};

std::vector<std::wstring> EnumerateComPorts();

}
