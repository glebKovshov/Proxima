#include "proxima/logging/logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace proxima::logging {

namespace {
std::string nowString() {
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::ostringstream output;
    output << std::put_time(&local, "%Y-%m-%dT%H:%M:%S");
    return output.str();
}
}

Logger::Logger(std::filesystem::path filePath) { setFilePath(std::move(filePath)); }
Logger::~Logger() = default;

void Logger::setFilePath(std::filesystem::path filePath) {
    std::lock_guard lock(mutex_);
    if (stream_.is_open()) stream_.close();
    filePath_ = std::move(filePath);
    if (filePath_.empty()) return;
    std::error_code error;
    if (!filePath_.parent_path().empty()) {
        std::filesystem::create_directories(filePath_.parent_path(), error);
    }
    stream_.open(filePath_, std::ios::app);
}

void Logger::log(const LogLevel level, const std::string& module, const std::string& message) {
    std::lock_guard lock(mutex_);
    if (!stream_.is_open()) return;
    stream_ << nowString() << " [" << toString(level) << "] [" << module << "] " << message << '\n';
    stream_.flush();
}

void Logger::trace(const std::string& module, const std::string& message) { log(LogLevel::Trace, module, message); }
void Logger::debug(const std::string& module, const std::string& message) { log(LogLevel::Debug, module, message); }
void Logger::info(const std::string& module, const std::string& message) { log(LogLevel::Info, module, message); }
void Logger::warning(const std::string& module, const std::string& message) { log(LogLevel::Warning, module, message); }
void Logger::error(const std::string& module, const std::string& message) { log(LogLevel::Error, module, message); }
void Logger::critical(const std::string& module, const std::string& message) { log(LogLevel::Critical, module, message); }
const std::filesystem::path& Logger::filePath() const noexcept { return filePath_; }

const char* toString(const LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Trace: return "TRACE";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARNING";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Critical: return "CRITICAL";
    }
    return "INFO";
}

} // namespace proxima::logging
