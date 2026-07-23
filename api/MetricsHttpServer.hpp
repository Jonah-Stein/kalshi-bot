#pragma once

#include "types.hpp"
#include "../infra/DoubleBuffer.hpp"

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

class MetricsHttpServer {
public:
    MetricsHttpServer(
        DoubleBuffer<OrderbookMetrics>& metrics,
        std::string bind_address = "0.0.0.0",
        std::uint16_t port = 8080
    );

    ~MetricsHttpServer();

    MetricsHttpServer(const MetricsHttpServer&) = delete;
    MetricsHttpServer& operator=(const MetricsHttpServer&) = delete;

    void start();
    void stop();

private:
    void run();

    DoubleBuffer<OrderbookMetrics>& metrics_;
    std::string bind_address_;
    std::uint16_t port_;
    std::atomic<bool> running_{false};
    std::thread worker_;
};
