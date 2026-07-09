#include "helpers.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <span>
#include <stdexcept>
#include <vector>



double percentile(
    const std::vector<std::int64_t>& sorted_values,
    double percentile_value
) {
    if (sorted_values.empty()) {
        throw std::invalid_argument("Cannot calculate percentile of empty data");
    }

    if (percentile_value < 0.0 || percentile_value > 1.0) {
        throw std::invalid_argument("Percentile must be between 0 and 1");
    }

    // Linear interpolation between adjacent samples.
    const double index =
        percentile_value * static_cast<double>(sorted_values.size() - 1);

    const std::size_t lower =
        static_cast<std::size_t>(std::floor(index));
    const std::size_t upper =
        static_cast<std::size_t>(std::ceil(index));

    if (lower == upper) {
        return static_cast<double>(sorted_values[lower]);
    }

    const double weight = index - static_cast<double>(lower);

    return static_cast<double>(sorted_values[lower]) * (1.0 - weight)
         + static_cast<double>(sorted_values[upper]) * weight;
}

LatencyStats calculate_latency_stats(
    std::span<const uint64_t> write_timings,
    std::span<const uint64_t> read_timings,
    std::size_t warmup_messages
) {
    if (write_timings.size() != read_timings.size()) {
        throw std::invalid_argument(
            "write_timings and read_timings must have the same size"
        );
    }

    if (warmup_messages >= write_timings.size()) {
        throw std::invalid_argument(
            "Warm-up count must be smaller than the total message count"
        );
    }

    LatencyStats stats;
    stats.warmup_messages = warmup_messages;

    std::vector<std::int64_t> latencies;
    latencies.reserve(write_timings.size() - warmup_messages);

    for (std::size_t i = warmup_messages; i < write_timings.size(); ++i) {
        // A read before its corresponding write indicates mismatched messages,
        // different clocks, timestamp corruption, or another measurement error.
        if (read_timings[i] < write_timings[i]) {
            ++stats.invalid_messages;
            continue;
        }

        latencies.push_back(read_timings[i] - write_timings[i]);
    }

    if (latencies.empty()) {
        throw std::runtime_error("No valid latency samples were collected");
    }

    stats.measured_messages = latencies.size();

    const long double sum = std::accumulate(
        latencies.begin(),
        latencies.end(),
        static_cast<long double>(0)
    );

    stats.mean_ns =
        static_cast<double>(sum / static_cast<long double>(latencies.size()));

    long double squared_error_sum = 0.0L;

    for (const std::int64_t latency : latencies) {
        const long double difference =
            static_cast<long double>(latency) - stats.mean_ns;

        squared_error_sum += difference * difference;
    }

    stats.stddev_ns = std::sqrt(
        static_cast<double>(
            squared_error_sum /
            static_cast<long double>(latencies.size())
        )
    );

    std::sort(latencies.begin(), latencies.end());

    stats.min_ns = latencies.front();
    stats.max_ns = latencies.back();
    stats.median_ns = percentile(latencies, 0.50);
    stats.p95_ns = percentile(latencies, 0.95);
    stats.p99_ns = percentile(latencies, 0.99);

    return stats;
}

void print_latency_stats(const LatencyStats& stats) {
    constexpr double ns_per_us = 1'000.0;

    std::cout << std::fixed << std::setprecision(3);

    std::cout << "Warm-up messages:  "
              << stats.warmup_messages << '\n';

    std::cout << "Measured messages: "
              << stats.measured_messages << '\n';

    std::cout << "Invalid messages:  "
              << stats.invalid_messages << "\n\n";

    std::cout << "Minimum:  "
              << stats.min_ns / ns_per_us << " us\n";

    std::cout << "Mean:     "
              << stats.mean_ns / ns_per_us << " us\n";

    std::cout << "Median:   "
              << stats.median_ns / ns_per_us << " us\n";

    std::cout << "p95:      "
              << stats.p95_ns / ns_per_us << " us\n";

    std::cout << "p99:      "
              << stats.p99_ns / ns_per_us << " us\n";

    std::cout << "Maximum:  "
              << stats.max_ns / ns_per_us << " us\n";

    std::cout << "Std. dev: "
              << stats.stddev_ns / ns_per_us << " us\n";
}