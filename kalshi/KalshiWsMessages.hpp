#pragma once

#include <cstdint>
#include <vector>

enum class KalshiSide : uint8_t { Yes, No};

struct KalshiPriceLevel {
    uint16_t price_hundredths_cents;
    uint16_t quantity;
};

struct KalshiOrderbookSnapshot {
    std::vector<KalshiPriceLevel> yes_levels;
    std::vector<KalshiPriceLevel> no_levels;
};

struct KalshiOrderbookDelta {
    uint16_t price_hundredths_cents;
    uint32_t quantity_hundredths;
    KalshiSide side;
    uint64_t ts_ms;
};
