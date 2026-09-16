#include "pch.h"
#include "SerialLink.h"

namespace LightBar {

namespace {

std::string WideToUtf8(std::wstring const& text) {
    if (text.empty()) return {};
    int size = ::WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), nullptr, 0, nullptr, nullptr);
    std::string out(size, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), out.data(), size, nullptr, nullptr);
    return out;
}

std::wstring Utf8ToWide(std::string const& text) {
    if (text.empty()) return {};
    int size = ::MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), nullptr, 0);
    std::wstring out(size, L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), out.data(), size);
    return out;
}

}

SerialLink::SerialLink(std::wstring portName) : port_(std::move(portName)) {}

SerialLink::~SerialLink() {
    Stop();
}

void SerialLink::Start() {
    if (running_.exchange(true)) return;
    worker_ = std::thread(&SerialLink::Worker, this);
}

void SerialLink::Stop() {
    if (!running_.exchange(false)) return;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (handle_ != INVALID_HANDLE_VALUE) {
            ::CancelIoEx(handle_, nullptr);
        }
    }
    if (worker_.joinable()) {
        worker_.join();
    }
}

void SerialLink::SetPort(std::wstring const& portName) {
    if (portName == port_) return;
    Stop();
    port_ = portName;
    Start();
}

void SerialLink::Send(std::wstring const& command) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ == INVALID_HANDLE_VALUE) return;
    std::string utf8 = WideToUtf8(command);
    utf8.push_back('\n');
    DWORD written = 0;
    ::WriteFile(handle_, utf8.data(), (DWORD)utf8.size(), &written, nullptr);
}

uint64_t SerialLink::AddLineHandler(LineHandler handler) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    const uint64_t token = nextHandlerToken_++;
    lineHandlers_.emplace_back(token, std::move(handler));
    return token;
}

void SerialLink::RemoveLineHandler(uint64_t token) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    lineHandlers_.erase(
        std::remove_if(lineHandlers_.begin(), lineHandlers_.end(),
                       [token](auto const& entry) { return entry.first == token; }),
        lineHandlers_.end());
}

bool SerialLink::OpenLocked() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ != INVALID_HANDLE_VALUE) return true;

    std::wstring path = L"\\\\.\\" + port_;
    HANDLE h = ::CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                             OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        return false;
    }

    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (!::GetCommState(h, &dcb)) { ::CloseHandle(h); return false; }
    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    if (!::SetCommState(h, &dcb)) { ::CloseHandle(h); return false; }

    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 1000;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    ::SetCommTimeouts(h, &timeouts);

    ::SetupComm(h, 4096, 4096);
    ::PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);

    handle_ = h;
    return true;
}

void SerialLink::CloseLocked() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (handle_ != INVALID_HANDLE_VALUE) {
        ::CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }
}

void SerialLink::Worker() {
    std::string buffer;

    while (running_.load()) {
        HANDLE h;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            h = handle_;
        }

        if (h == INVALID_HANDLE_VALUE) {
            if (OpenLocked()) {
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    h = handle_;
                }
                buffer.clear();
                if (statusHandler_) statusHandler_(true, L"Connected to " + port_);
            } else {
                if (statusHandler_) statusHandler_(false, L"Searching for " + port_ + L"...");
                ::Sleep(1500);
                continue;
            }
        }

        char chunk[512];
        DWORD read = 0;
        BOOL ok = ::ReadFile(h, chunk, sizeof(chunk), &read, nullptr);
        if (!ok) {
            if (!running_.load()) break;
            CloseLocked();
            if (statusHandler_) statusHandler_(false, L"Disconnected - reconnecting...");
            ::Sleep(200);
            continue;
        }

        if (read > 0) {
            buffer.append(chunk, read);
            size_t pos;
            while ((pos = buffer.find('\n')) != std::string::npos) {
                std::string line = buffer.substr(0, pos);
                buffer.erase(0, pos + 1);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (!line.empty()) {
                    std::vector<LineHandler> handlers;
                    {
                        std::lock_guard<std::mutex> lock(handlersMutex_);
                        handlers.reserve(lineHandlers_.size());
                        for (auto const& entry : lineHandlers_) handlers.push_back(entry.second);
                    }
                    const std::wstring wide = Utf8ToWide(line);
                    for (auto const& handler : handlers) handler(wide);
                }
            }
            if (buffer.size() > 8192) buffer.clear();
        }
    }

    CloseLocked();
}

std::vector<std::wstring> EnumerateComPorts() {
    std::vector<std::wstring> ports;

    HKEY key = nullptr;
    if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ,
                        &key) != ERROR_SUCCESS) {
        return ports;
    }

    DWORD index = 0;
    wchar_t name[256];
    BYTE data[256];
    while (true) {
        DWORD nameLength = 256;
        DWORD dataLength = sizeof(data);
        DWORD type = 0;
        if (::RegEnumValueW(key, index++, name, &nameLength, nullptr, &type, data, &dataLength) !=
            ERROR_SUCCESS) {
            break;
        }
        if (type == REG_SZ) {
            ports.emplace_back(reinterpret_cast<wchar_t*>(data));
        }
    }
    ::RegCloseKey(key);

    std::sort(ports.begin(), ports.end());
    return ports;
}

}
