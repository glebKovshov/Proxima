#include "proxima/diagnostics/traceroute_probe.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <Windows.h>
#include <IPExport.h>
#include <IcmpAPI.h>
#include <WS2tcpip.h>
#endif

#include <algorithm>
#include <array>
#include <cstring>

namespace proxima::diagnostics {

std::vector<TracerouteHop> TracerouteProbe::run(
    const std::string& host,
    const int maxHops,
    const std::chrono::milliseconds timeout) const {
    std::vector<TracerouteHop> hops;
    const int boundedHops = std::clamp(maxHops, 1, 32);

#ifdef _WIN32
    addrinfo hints{};
    hints.ai_family = AF_INET;
    addrinfo* resolved = nullptr;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &resolved) != 0 || resolved == nullptr) {
        return hops;
    }
    const IPAddr destination = reinterpret_cast<const sockaddr_in*>(resolved->ai_addr)->sin_addr.S_un.S_addr;
    IN_ADDR destinationAddress{};
    destinationAddress.S_un.S_addr = destination;
    char destinationText[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &destinationAddress, destinationText, sizeof(destinationText));
    HANDLE icmp = IcmpCreateFile();
    if (icmp == INVALID_HANDLE_VALUE) {
        freeaddrinfo(resolved);
        return hops;
    }

    const char payload[] = "proxima-trace";
    constexpr DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(payload) + 64;
    std::array<std::byte, replySize> replyBuffer{};
    for (int currentHop = 1; currentHop <= boundedHops; ++currentHop) {
        IP_OPTION_INFORMATION options{};
        options.Ttl = static_cast<unsigned char>(currentHop);
        const auto begin = std::chrono::steady_clock::now();
        const DWORD result = IcmpSendEcho(
            icmp,
            destination,
            const_cast<char*>(payload),
            static_cast<WORD>(sizeof(payload)),
            &options,
            replyBuffer.data(),
            replySize,
            static_cast<DWORD>(std::max<std::int64_t>(1, timeout.count())));
        TracerouteHop hop;
        hop.hop = currentHop;
        if (result > 0) {
            const auto* reply = reinterpret_cast<const ICMP_ECHO_REPLY*>(replyBuffer.data());
            IN_ADDR address{};
            address.S_un.S_addr = reply->Address;
            char ip[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, &address, ip, sizeof(ip));
            hop.ip = ip;
            hop.latencyMs = static_cast<double>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - begin).count()) / 1000.0;
            hop.timeout = false;
            if (reply->Status == IP_SUCCESS && hop.ip == destinationText) {
                hops.push_back(hop);
                break;
            }
        }
        hops.push_back(std::move(hop));
    }
    IcmpCloseHandle(icmp);
    freeaddrinfo(resolved);
#else
    (void)host;
    (void)timeout;
    for (int currentHop = 1; currentHop <= boundedHops; ++currentHop) {
        hops.push_back(TracerouteHop{currentHop, "*", {}, std::nullopt, true});
    }
#endif
    return hops;
}

} // namespace proxima::diagnostics
