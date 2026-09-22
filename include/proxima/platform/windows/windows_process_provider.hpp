#pragma once

#include "proxima/discovery/fortnite_process_detector.hpp"

namespace proxima::platform::windows {

class WindowsProcessProvider final : public discovery::IProcessProvider {
public:
    [[nodiscard]] std::vector<discovery::ProcessInfo> listProcesses() const override;
};

} // namespace proxima::platform::windows
