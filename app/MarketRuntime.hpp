#pragma once

#include "../api/types.hpp"
#include "../infra/DoubleBuffer.hpp"
#include "../infra/ObjectRingBuffer.hpp"
#include "../kalshi/KalshiAuth.hpp"
#include "../kalshi/KalshiMessageParser.hpp"
#include "../kalshi/KalshiWsClient.hpp"
#include "../orderbook/Orderbook.hpp"

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

class MarketRuntime {
public:
    explicit MarketRuntime(
        DoubleBuffer<OrderbookMetrics>& metrics,
        KalshiAuth& auth,
        std::string& ws_host,
        std::string& connection_path,
        uint16_t price_denominations = 100
    );

    void start(std::string& ticker);
    void stop();

private:
    ObjectRingBuffer ring_buffer_;
    KalshiWsClient kalshi_ws_client_;
    Orderbook orderbook_;
    DoubleBuffer<OrderbookMetrics>& metrics_buffer_;
    KalshiMessageParser parser_;

    std::atomic<bool> running_{false};

    // metrics
    uint64_t messages_received_;
    uint64_t messages_processed_;

    std::thread orderbook_thread_;

    // called from kalshi ws client
    void handle_message(std::string& msg);

    void report_metrics();
    // runs on orderbook_thread_
    void consume_loop();
};
