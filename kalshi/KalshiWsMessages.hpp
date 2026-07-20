#pragma once

#include <cstdint>
#include <vector>

enum class KalshiSide : uint8_t { Yes, No};
inline std::string_view toString(KalshiSide side) {
    switch (side) {
        case KalshiSide::Yes: return "yes";
        case KalshiSide::No: return "no";
    }
}



struct KalshiPriceLevel {
    uint16_t price;
    uint32_t quantity;
};

struct KalshiOrderbookSnapshot {
    std::vector<KalshiPriceLevel> yes_levels;
    std::vector<KalshiPriceLevel> no_levels;
};

struct KalshiOrderbookDelta {
    uint16_t price;
    uint32_t quantity_hundredths;
    KalshiSide side;
    uint64_t ts_ms;
};

enum class KalshiMessageType : uint8_t {Snapshot, Delta, Stop};
