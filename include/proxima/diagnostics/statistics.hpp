#pragma once

#include <cstddef>
#include <optional>
#include <vector>

namespace proxima::diagnostics {

class RollingStatistics {
public:
    explicit RollingStatistics(std::size_t capacity = 120);

    void add(double value);
    void reset();
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::optional<double> min() const;
    [[nodiscard]] std::optional<double> max() const;
    [[nodiscard]] std::optional<double> mean() const;
    [[nodiscard]] std::optional<double> standardDeviation() const;
    [[nodiscard]] const std::vector<double>& values() const noexcept;

private:
    std::size_t capacity_;
    std::vector<double> values_;
};

class JitterCalculator {
public:
    static double calculate(const std::vector<double>& samples);
};

class PacketLossAnalyzer {
public:
    void recordSuccess();
    void recordFailure();
    void record(bool success);
    void reset();
    [[nodiscard]] std::size_t transmitted() const noexcept;
    [[nodiscard]] std::size_t lost() const noexcept;
    [[nodiscard]] double lossPercent() const noexcept;

private:
    std::size_t transmitted_{0};
    std::size_t lost_{0};
};

} // namespace proxima::diagnostics
