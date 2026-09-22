#include "proxima/diagnostics/ping_probe.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <Windows.h>
#include <IPExport.h>
#include <IcmpAPI.h>
#include <WS2tcpip.h>

namespace {
class WinsockSession final {
public:
    WinsockSession() {
        WSADATA data{};
        initialized_ = WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }

    ~WinsockSession() {
        if (initialized_) WSACleanup();
    }

    [[nodiscard]] bool initialized() const noexcept { return initialized_; }

private:
    bool initialized_{false};
};
}
#endif

namespace proxima::diagnostics {

std::optional<double> IcmpPingProbe::measure(
    const std::string& host,
    const std::chrono::milliseconds timeout) {
#ifdef _WIN32
    WinsockSession winsock;
    if (!winsock.initialized()) return std::nullopt;

    addrinfo hints{};
    hints.ai_family = AF_INET;
    addrinfo* resolved = nullptr;
    const auto addressInfoResult = getaddrinfo(host.c_str(), nullptr, &hints, &resolved);
    if (addressInfoResult != 0 || resolved == nullptr) {
        return std::nullopt;
    }

    const auto* address = reinterpret_cast<const sockaddr_in*>(resolved->ai_addr);
    const IPAddr destination = address->sin_addr.S_un.S_addr;
    HANDLE icmp = IcmpCreateFile();
    if (icmp == INVALID_HANDLE_VALUE) {
        freeaddrinfo(resolved);
        return std::nullopt;
    }

    const char payload[] = "proxima";
    constexpr DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(payload) + 32;
    std::vector<std::byte> replyBuffer(replySize);
    const DWORD waitMs = static_cast<DWORD>(std::max<std::int64_t>(1, timeout.count()));
    const DWORD result = IcmpSendEcho(
        icmp,
        destination,
        const_cast<char*>(payload),
        static_cast<WORD>(sizeof(payload)),
        nullptr,
        replyBuffer.data(),
        replySize,
        waitMs);

    std::optional<double> resultMs;
    if (result > 0) {
        const auto* reply = reinterpret_cast<const ICMP_ECHO_REPLY*>(replyBuffer.data());
        if (reply->Status == IP_SUCCESS) {
            resultMs = static_cast<double>(reply->RoundTripTime);
        }
    }

    IcmpCloseHandle(icmp);
    freeaddrinfo(resolved);
    return resultMs;
#else
    (void)host;
    (void)timeout;
    return std::nullopt;
#endif
}

} // namespace proxima::diagnostics
