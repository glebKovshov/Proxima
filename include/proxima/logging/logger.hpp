#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

namespace proxima::logging {

enum class LogLevel { Trace, Debug, Info, Warning, Error, Critical };

class Logger {
public:
    explicit Logger(std::filesystem::path filePath = {});
    ~Logger();

    void setFilePath(std::filesystem::path filePath);
    void log(LogLevel level, const std::string& module, const std::string& message);
    void trace(const std::string& module, const std::string& message);
    void debug(const std::string& module, const std::string& message);
    void info(const std::string& module, const std::string& message);
    void warning(const std::string& module, const std::string& message);
    void error(const std::string& module, const std::string& message);
    void critical(const std::string& module, const std::string& message);
    [[nodiscard]] const std::filesystem::path& filePath() const noexcept;

private:
    std::filesystem::path filePath_;
    std::ofstream stream_;
    std::mutex mutex_;
};

[[nodiscard]] const char* toString(LogLevel level) noexcept;

} // namespace proxima::logging
