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
    void printSnapshot();

    std::vector<uint32_t> getSnapshot() const;
    uint16_t best_bid() const { return bid_price_level_; }
    uint16_t best_ask() const { return ask_price_level_; }
    uint32_t size_at(uint16_t price) const {
        return contracts_at_price_[price];
    }
private:
    std::vector<uint32_t> contracts_at_price_;
    uint16_t ask_price_level_;
    uint16_t bid_price_level_;
    uint16_t price_denominations_;

    void applyNoDelta(KalshiOrderbookDelta& delta);
    void applyYesDelta(KalshiOrderbookDelta& delta);
};