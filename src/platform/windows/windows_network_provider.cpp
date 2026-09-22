#include "proxima/platform/windows/windows_network_provider.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <Windows.h>
#include <iphlpapi.h>
#include <WS2tcpip.h>
#endif

#include <chrono>
#include <cstddef>
#include <vector>

namespace proxima::platform::windows {

std::vector<core::Endpoint> WindowsNetworkProvider::endpointsForProcess(const std::uint32_t pid) const {
    std::vector<core::Endpoint> result;
#ifdef _WIN32
    ULONG size = 0;
    GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    std::vector<std::byte> buffer(size);
    auto* table = reinterpret_cast<MIB_TCPTABLE_OWNER_PID*>(buffer.data());
    if (GetExtendedTcpTable(table, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
        for (DWORD index = 0; index < table->dwNumEntries; ++index) {
            const auto& row = table->table[index];
            if (row.dwOwningPid != pid) continue;
            IN_ADDR address{};
            address.S_un.S_addr = row.dwRemoteAddr;
            char remoteAddress[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, &address, remoteAddress, sizeof(remoteAddress));
            const auto remotePort = ntohs(static_cast<u_short>(row.dwRemotePort));
            if (remoteAddress[0] == '\0' || remotePort == 0) continue;
            core::Endpoint endpoint;
            endpoint.address = remoteAddress;
            endpoint.port = remotePort;
            endpoint.protocol = "tcp";
            endpoint.lastSeen = std::chrono::system_clock::now();
            endpoint.activityCount = 1;
            endpoint.activityPerSecond = 0.0;
            endpoint.likelyGameEndpoint = remotePort >= 1024;
            result.push_back(std::move(endpoint));
        }
    }

    ULONG udpSize = 0;
    GetExtendedUdpTable(nullptr, &udpSize, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0);
    std::vector<std::byte> udpBuffer(udpSize);
    auto* udpTable = reinterpret_cast<MIB_UDPTABLE_OWNER_PID*>(udpBuffer.data());
    if (GetExtendedUdpTable(udpTable, &udpSize, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) == NO_ERROR) {
        for (DWORD index = 0; index < udpTable->dwNumEntries; ++index) {
            const auto& row = udpTable->table[index];
            if (row.dwOwningPid != pid) continue;
            core::Endpoint endpoint;
            endpoint.address = "0.0.0.0";
            endpoint.port = ntohs(static_cast<u_short>(row.dwLocalPort));
            endpoint.protocol = "udp";
            endpoint.lastSeen = std::chrono::system_clock::now();
            endpoint.activityCount = 1;
            endpoint.activityPerSecond = 0.0;
            endpoint.likelyGameEndpoint = false;
            result.push_back(std::move(endpoint));
        }
    }
#else
    (void)pid;
#endif
    return result;
}

} // namespace proxima::platform::windows
