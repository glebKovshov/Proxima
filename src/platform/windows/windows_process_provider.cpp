#include "proxima/platform/windows/windows_process_provider.hpp"

#include <chrono>

#ifdef _WIN32
#include <Windows.h>
#include <TlHelp32.h>
#endif

namespace proxima::platform::windows {

std::vector<discovery::ProcessInfo> WindowsProcessProvider::listProcesses() const {
    std::vector<discovery::ProcessInfo> result;
#ifdef _WIN32
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return result;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry) != FALSE) {
        do {
            char executable[512]{};
            WideCharToMultiByte(CP_UTF8, 0, entry.szExeFile, -1, executable, sizeof(executable), nullptr, nullptr);
            result.push_back({entry.th32ProcessID, executable, std::chrono::system_clock::now()});
        } while (Process32NextW(snapshot, &entry) != FALSE);
    }
    CloseHandle(snapshot);
#endif
    return result;
}

} // namespace proxima::platform::windows
