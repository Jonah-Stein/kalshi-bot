#pragma once

#include "types.hpp"

#include <format>
#include <string>

inline std::string to_json(const OrderbookMetrics& m) {
    return std::format(
        R"({{"messages_received":{},"bid_price":{},"bid_quantity":{},"ask_price":{},"ask_quantity":{}}})",
        m.messages_received,
        m.bid_price,
        m.bid_quantity,
        m.ask_price,
        m.ask_quantity
    );
}
