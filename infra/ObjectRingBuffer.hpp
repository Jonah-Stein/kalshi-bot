#pragma once
#include "../helpers/helpers.hpp"
#include "../kalshi/KalshiWsMessages.hpp"

#include <atomic>
#include <cstdint>
#include <vector>
#include <stdexcept>

#define CACHE_LINE_SIZE 64

#ifndef FORCE_INLINE
#   if defined(_MSC_VER)
#       define FORCE_INLINE __forceinline
#   elif defined(__GNUC__)
#       define FORCE_INLINE inline __attribute__ ((always_inline))
#   else
#       define FORCE_INLINE inline
#   endif
#endif

struct OrderbookUpdate {
    KalshiMessageType type;
    // std::string ticker; TODO
    KalshiOrderbookDelta delta;
    KalshiOrderbookSnapshot snapshot;
};

class ObjectRingBuffer{
public:
    ObjectRingBuffer(uint32_t slots, uint32_t price_levels = 100){
        if (slots == 0 || (slots & (slots - 1)) != 0){
            throw std::invalid_argument("slot must be nonzero power of 2");
        }
    
        buffer_.resize(slots);
        buffer_size_ = slots;
        // initial slot capacity becomes useful for writing directly
        // from beast buffer
        for (OrderbookUpdate& slot: buffer_){
            slot.snapshot.yes_levels.reserve(price_levels);
            slot.snapshot.no_levels.reserve(price_levels);
        }
    }

    FORCE_INLINE OrderbookUpdate* GetWriteSlot();
    FORCE_INLINE void FinishWrite();

    FORCE_INLINE bool TryWriteDelta(uint16_t price, uint32_t quantity_hundredths, KalshiSide side, uint64_t ts_ms);
    FORCE_INLINE bool TryWriteSnapshot(const std::vector<KalshiPriceLevel>& yes_levels, const std::vector<KalshiPriceLevel>& no_levels);

    FORCE_INLINE bool TryWriteDeltaWithTiming(uint16_t price, uint32_t quantity_hundredths, KalshiSide side, uint64_t ts_ms, std::vector<uint64_t>& timings);
    FORCE_INLINE bool TryWriteSnapshotWithTiming(const std::vector<KalshiPriceLevel>& yes_levels, const std::vector<KalshiPriceLevel>& no_levels, std::vector<uint64_t>& timings);
    
    FORCE_INLINE OrderbookUpdate* TryRead();
    FORCE_INLINE void FinishRead();

private:
    // these values are monotonic counters
    struct alignas(CACHE_LINE_SIZE) ProducerSide {
        std::uint64_t counts = 0;
        std::atomic<std::uint64_t> published_counts{0};
    };
    struct alignas(CACHE_LINE_SIZE) ConsumerSide {
        std::uint64_t counts = 0;
        std::atomic<std::uint64_t> published_counts{0};
    };
    
    ProducerSide producer_;
    ConsumerSide consumer_;

    uint32_t buffer_size_;
    std::vector<OrderbookUpdate> buffer_;
};

OrderbookUpdate* ObjectRingBuffer::GetWriteSlot(){
    if (producer_.counts - consumer_.published_counts.load(std::memory_order_acquire) >= buffer_size_){
        return nullptr;
    }
    return &buffer_[producer_.counts & (buffer_size_ - 1)];
}
void ObjectRingBuffer::FinishWrite(){
    producer_.counts++;
    producer_.published_counts.store(producer_.counts, std::memory_order_release);
}

bool ObjectRingBuffer::TryWriteDelta(uint16_t price, uint32_t quantity_hundredths, KalshiSide side, uint64_t ts_ms){
    if (producer_.counts - consumer_.published_counts.load(std::memory_order_acquire) >= buffer_size_){
        return false;
    }
    OrderbookUpdate& slot = buffer_[producer_.counts & (buffer_size_ - 1)];
    slot.type = KalshiMessageType::Delta;
    slot.delta.price = price;
    slot.delta.quantity_hundredths = quantity_hundredths;
    slot.delta.side = side;
    slot.delta.ts_ms = ts_ms;

    producer_.counts++;
    producer_.published_counts.store(producer_.counts, std::memory_order_release);
    return true;
}

bool ObjectRingBuffer::TryWriteSnapshot(const std::vector<KalshiPriceLevel>& yes_levels, const std::vector<KalshiPriceLevel>& no_levels){
    if (producer_.counts - consumer_.published_counts.load(std::memory_order_acquire) >= buffer_size_){
        return false;
    }
    OrderbookUpdate& slot = buffer_[producer_.counts & (buffer_size_ - 1)];
    slot.type = KalshiMessageType::Snapshot;
    slot.snapshot.yes_levels.assign(yes_levels.begin(), yes_levels.end());
    slot.snapshot.no_levels.assign(no_levels.begin(), no_levels.end());

    producer_.counts++;
    producer_.published_counts.store(producer_.counts, std::memory_order_release);
    return true;
}

bool ObjectRingBuffer::TryWriteDeltaWithTiming(uint16_t price, uint32_t quantity_hundredths, KalshiSide side, uint64_t ts_ms, std::vector<uint64_t>& timings){
    if (producer_.counts - consumer_.published_counts.load(std::memory_order_acquire) >= buffer_size_){
        return false;
    }
    timings.push_back(timestampNs());

    OrderbookUpdate& slot = buffer_[producer_.counts & (buffer_size_ - 1)];
    slot.type = KalshiMessageType::Delta;
    slot.delta.price = price;
    slot.delta.quantity_hundredths = quantity_hundredths;
    slot.delta.side = side;
    slot.delta.ts_ms = ts_ms;

    producer_.counts++;
    producer_.published_counts.store(producer_.counts, std::memory_order_release);
    return true;
}

bool ObjectRingBuffer::TryWriteSnapshotWithTiming(const std::vector<KalshiPriceLevel>& yes_levels, const std::vector<KalshiPriceLevel>& no_levels, std::vector<uint64_t>& timings){
    if (producer_.counts - consumer_.published_counts.load(std::memory_order_acquire) >= buffer_size_){
        return false;
    }
    timings.push_back(timestampNs());

    OrderbookUpdate& slot = buffer_[producer_.counts & (buffer_size_ - 1)];
    slot.type = KalshiMessageType::Snapshot;
    slot.snapshot.yes_levels.assign(yes_levels.begin(), yes_levels.end());
    slot.snapshot.no_levels.assign(no_levels.begin(), no_levels.end());

    producer_.counts++;
    producer_.published_counts.store(producer_.counts, std::memory_order_release);
    return true;
}

OrderbookUpdate* ObjectRingBuffer::TryRead(){
    if (consumer_.counts == producer_.published_counts.load(std::memory_order_acquire)){
        return nullptr;
    }

    return &buffer_[consumer_.counts & (buffer_size_ - 1)];
}

void ObjectRingBuffer::FinishRead(){
    consumer_.counts++;
    consumer_.published_counts.store(consumer_.counts, std::memory_order_release);
}