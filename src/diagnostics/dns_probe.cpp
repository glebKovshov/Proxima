#include "proxima/diagnostics/dns_probe.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#endif

#include <chrono>

namespace proxima::diagnostics {

DnsResult DnsProbe::resolve(const std::string& hostname, const std::chrono::milliseconds timeout) const {
    DnsResult result;
    result.hostname = hostname;
    const auto begin = std::chrono::steady_clock::now();
#ifdef _WIN32
    (void)timeout;
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    addrinfo* resolved = nullptr;
    const int error = getaddrinfo(hostname.c_str(), nullptr, &hints, &resolved);
    if (error != 0) {
        result.error = gai_strerrorA(error);
        return result;
    }
    for (addrinfo* item = resolved; item != nullptr; item = item->ai_next) {
        char address[INET6_ADDRSTRLEN]{};
        const void* source = item->ai_family == AF_INET
            ? static_cast<const void*>(&reinterpret_cast<sockaddr_in*>(item->ai_addr)->sin_addr)
            : static_cast<const void*>(&reinterpret_cast<sockaddr_in6*>(item->ai_addr)->sin6_addr);
        if (inet_ntop(item->ai_family, source, address, sizeof(address)) != nullptr) {
            result.addresses.emplace_back(address);
        }
    }
    freeaddrinfo(resolved);
    result.success = !result.addresses.empty();
#else
    (void)timeout;
    result.error = "DNS probe is only implemented for Windows in v0.1";
#endif
    result.resolveLatencyMs = static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - begin).count()) / 1000.0;
    return result;
}

} // namespace proxima::diagnostics
