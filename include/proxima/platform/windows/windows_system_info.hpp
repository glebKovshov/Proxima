#pragma once

#include <cstdint>
#include <string>

namespace proxima::platform::windows {

struct SystemInfo {
    std::string operatingSystem{"Windows"};
    std::string architecture{"x64"};
    std::uint64_t memoryMb{0};
};

class WindowsSystemInfo {
public:
    [[nodiscard]] static SystemInfo collect();
};

} // namespace proxima::platform::windows
