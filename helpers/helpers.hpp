#pragma once
#include <cstdint>
#include <span>
#include <cstddef>

// timestamps
uint64_t timestampMs();
uint64_t timestampNs();

// latency stats
struct LatencyStats {
    std::size_t measured_messages = 0;
    std::size_t warmup_messages = 0;
    std::size_t invalid_messages = 0;

    double mean_ns = 0.0;
    double median_ns = 0.0;
    double p95_ns = 0.0;
    double p99_ns = 0.0;
    double stddev_ns = 0.0;

    std::int64_t min_ns = 0;
    std::int64_t max_ns = 0;
};

LatencyStats calculate_latency_stats(
    std::span<const uint64_t> write_timings,
    std::span<const uint64_t> read_timings,
    std::size_t warmup_messages = 10'000
);

void print_latency_stats(const LatencyStats& stats);