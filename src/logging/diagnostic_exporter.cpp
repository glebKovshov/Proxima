#include "proxima/logging/diagnostic_exporter.hpp"

#include <array>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>

namespace proxima::logging {

namespace {
struct ZipEntry {
    std::string name;
    std::string data;
    std::uint32_t crc{0};
    std::uint32_t offset{0};
};

std::uint32_t crc32(const std::string& data) {
    std::uint32_t crc = 0xFFFFFFFFU;
    for (const unsigned char byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            const auto mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return ~crc;
}

void write16(std::ofstream& stream, const std::uint16_t value) {
    stream.put(static_cast<char>(value & 0xFFU));
    stream.put(static_cast<char>((value >> 8U) & 0xFFU));
}

void write32(std::ofstream& stream, const std::uint32_t value) {
    write16(stream, static_cast<std::uint16_t>(value & 0xFFFFU));
    write16(stream, static_cast<std::uint16_t>((value >> 16U) & 0xFFFFU));
}

std::string timestampName() {
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::ostringstream output;
    output << std::put_time(&local, "%Y-%m-%d-%H-%M");
    return output.str();
}
}

std::optional<std::filesystem::path> DiagnosticExporter::exportZip(
    const std::filesystem::path& directory,
    const DiagnosticSnapshot& snapshot,
    std::string* error) {
    const auto fail = [&](const std::string& message) -> std::optional<std::filesystem::path> {
        if (error) *error = message;
        return std::nullopt;
    };
    std::error_code filesystemError;
    std::filesystem::create_directories(directory, filesystemError);
    if (filesystemError) return fail("unable to create export directory");
    const auto outputPath = directory / ("proxima-diagnostics-" + timestampName() + ".zip");
    std::ofstream stream(outputPath, std::ios::binary | std::ios::trunc);
    if (!stream) return fail("unable to open diagnostic archive");

    const std::array<ZipEntry, 6> entries{{
        {"app.log", snapshot.appLog},
        {"network.json", snapshot.networkJson},
        {"routes.json", snapshot.routesJson},
        {"traceroute.txt", snapshot.tracerouteText},
        {"system.json", snapshot.systemJson},
        {"version.txt", snapshot.versionText}
    }};
    std::vector<ZipEntry> written;
    written.reserve(entries.size());
    for (auto entry : entries) {
        entry.offset = static_cast<std::uint32_t>(stream.tellp());
        entry.crc = crc32(entry.data);
        write32(stream, 0x04034B50U);
        write16(stream, 20);
        write16(stream, 0);
        write16(stream, 0);
        write16(stream, 0);
        write16(stream, 0);
        write32(stream, entry.crc);
        write32(stream, static_cast<std::uint32_t>(entry.data.size()));
        write32(stream, static_cast<std::uint32_t>(entry.data.size()));
        write16(stream, static_cast<std::uint16_t>(entry.name.size()));
        write16(stream, 0);
        stream.write(entry.name.data(), static_cast<std::streamsize>(entry.name.size()));
        stream.write(entry.data.data(), static_cast<std::streamsize>(entry.data.size()));
        written.push_back(std::move(entry));
    }

    const auto centralDirectoryOffset = static_cast<std::uint32_t>(stream.tellp());
    for (const auto& entry : written) {
        write32(stream, 0x02014B50U);
        write16(stream, 20);
        write16(stream, 20);
        write16(stream, 0);
        write16(stream, 0);
        write16(stream, 0);
        write16(stream, 0);
        write32(stream, entry.crc);
        write32(stream, static_cast<std::uint32_t>(entry.data.size()));
        write32(stream, static_cast<std::uint32_t>(entry.data.size()));
        write16(stream, static_cast<std::uint16_t>(entry.name.size()));
        write16(stream, 0);
        write16(stream, 0);
        write16(stream, 0);
        write16(stream, 0);
        write32(stream, 0);
        write32(stream, entry.offset);
        stream.write(entry.name.data(), static_cast<std::streamsize>(entry.name.size()));
    }
    const auto centralDirectorySize = static_cast<std::uint32_t>(stream.tellp()) - centralDirectoryOffset;
    write32(stream, 0x06054B50U);
    write16(stream, 0);
    write16(stream, 0);
    write16(stream, static_cast<std::uint16_t>(written.size()));
    write16(stream, static_cast<std::uint16_t>(written.size()));
    write32(stream, centralDirectorySize);
    write32(stream, centralDirectoryOffset);
    write16(stream, 0);
    if (!stream.good()) return fail("unable to finalize diagnostic archive");
    return outputPath;
}

std::string DiagnosticExporter::tracerouteToText(const std::vector<diagnostics::TracerouteHop>& hops) {
    std::ostringstream output;
    output << "Hop\tIP\tHostname\tLatency\tStatus\n";
    for (const auto& hop : hops) {
        output << hop.hop << '\t' << (hop.ip.empty() ? "*" : hop.ip) << '\t'
               << (hop.hostname.empty() ? "-" : hop.hostname) << '\t';
        if (hop.latencyMs) output << *hop.latencyMs << " ms";
        else output << '*';
        output << '\t' << (hop.timeout ? "Timeout" : "OK") << '\n';
    }
    return output.str();
}

std::string DiagnosticExporter::metricsToJson(const core::RouteMetrics& metrics) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(2)
           << "{\"routeId\":\"" << metrics.routeId << "\","
           << "\"routeType\":\"" << core::toString(metrics.routeType) << "\","
           << "\"rttCurrentMs\":" << metrics.rttCurrentMs << ','
           << "\"rttMinMs\":" << metrics.rttMinMs << ','
           << "\"rttAvgMs\":" << metrics.rttAvgMs << ','
           << "\"rttMaxMs\":" << metrics.rttMaxMs << ','
           << "\"jitterMs\":" << metrics.jitterMs << ','
           << "\"packetLossPct\":" << metrics.packetLossPct << ','
           << "\"stabilityScore\":" << metrics.stabilityScore << ','
           << "\"sampleCount\":" << metrics.sampleCount << ','
           << "\"confidence\":" << metrics.confidence << ','
           << "\"qualityScore\":" << metrics.qualityScore << ','
           << "\"health\":\"" << core::toString(metrics.health) << "\"}";
    return output.str();
}

} // namespace proxima::logging
