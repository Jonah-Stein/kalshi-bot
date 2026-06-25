#pragma once

#include "../kalshi/KalshiWsMessages.hpp"
#include <queue>
#include <vector>
#include <cstdint>
#include <functional>

// yes side will alway be bids
// 100 - no will be asks
class Orderbook {
public:
    Orderbook(uint16_t price_denominations);
    void applyDelta(KalshiOrderbookDelta& delta);
    void applySnapshot(KalshiOrderbookSnapshot& snapshot);

private:
    std::vector<uint32_t> contracts_at_price_;
    std::priority_queue<uint16_t> bids_;    
    std::priority_queue<uint16_t, std::vector<uint16_t>, std::greater<uint16_t>> asks_;

    void applyNoDelta(KalshiOrderbookDelta& delta);
    void applyYesDelta(KalshiOrderbookDelta& delta);
};