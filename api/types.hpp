#pragma once

#include <cstdint>

struct OrderbookMetrics {
    uint64_t messages_received = 0;
    uint64_t messages_processed = 0;
    uint16_t bid_price = 0;
    uint32_t bid_quantity = 0;
    uint16_t ask_price = 0;
    uint32_t ask_quantity = 0;
};
