#include "proxima/platform/windows/windows_system_info.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace proxima::platform::windows {

SystemInfo WindowsSystemInfo::collect() {
    SystemInfo result;
#ifdef _WIN32
    MEMORYSTATUSEX memory{};
    memory.dwLength = sizeof(memory);
    if (GlobalMemoryStatusEx(&memory) != FALSE) {
        result.memoryMb = memory.ullTotalPhys / (1024ULL * 1024ULL);
    }
#endif
    return result;
}

} // namespace proxima::platform::windows
