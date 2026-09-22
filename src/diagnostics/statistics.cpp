#include "proxima/diagnostics/statistics.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace proxima::diagnostics {

RollingStatistics::RollingStatistics(const std::size_t capacity)
    : capacity_(std::max<std::size_t>(1, capacity)) {
    values_.reserve(capacity_);
}

void RollingStatistics::add(const double value) {
    if (!std::isfinite(value) || value < 0.0) {
        return;
    }
    if (values_.size() == capacity_) {
        values_.erase(values_.begin());
    }
    values_.push_back(value);
}

void RollingStatistics::reset() { values_.clear(); }

std::size_t RollingStatistics::size() const noexcept { return values_.size(); }
bool RollingStatistics::empty() const noexcept { return values_.empty(); }

std::optional<double> RollingStatistics::min() const {
    if (values_.empty()) return std::nullopt;
    return *std::min_element(values_.begin(), values_.end());
}

std::optional<double> RollingStatistics::max() const {
    if (values_.empty()) return std::nullopt;
    return *std::max_element(values_.begin(), values_.end());
}

std::optional<double> RollingStatistics::mean() const {
    if (values_.empty()) return std::nullopt;
    return std::accumulate(values_.begin(), values_.end(), 0.0) / static_cast<double>(values_.size());
}

std::optional<double> RollingStatistics::standardDeviation() const {
    if (values_.empty()) return std::nullopt;
    const double average = *mean();
    double squared = 0.0;
    for (const double value : values_) {
        const double delta = value - average;
        squared += delta * delta;
    }
    return std::sqrt(squared / static_cast<double>(values_.size()));
}

const std::vector<double>& RollingStatistics::values() const noexcept { return values_; }

double JitterCalculator::calculate(const std::vector<double>& samples) {
    if (samples.size() < 2) return 0.0;
    double totalDelta = 0.0;
    for (std::size_t i = 1; i < samples.size(); ++i) {
        totalDelta += std::abs(samples[i] - samples[i - 1]);
    }
    return totalDelta / static_cast<double>(samples.size() - 1);
}

void PacketLossAnalyzer::recordSuccess() { ++transmitted_; }
void PacketLossAnalyzer::recordFailure() { ++transmitted_; ++lost_; }
void PacketLossAnalyzer::record(const bool success) { success ? recordSuccess() : recordFailure(); }
void PacketLossAnalyzer::reset() { transmitted_ = 0; lost_ = 0; }
std::size_t PacketLossAnalyzer::transmitted() const noexcept { return transmitted_; }
std::size_t PacketLossAnalyzer::lost() const noexcept { return lost_; }

double PacketLossAnalyzer::lossPercent() const noexcept {
    if (transmitted_ == 0) return 0.0;
    return 100.0 * static_cast<double>(lost_) / static_cast<double>(transmitted_);
}

} // namespace proxima::diagnostics
