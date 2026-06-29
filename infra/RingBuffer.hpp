#include <atomic>
#include <algorithm>
#include <string>
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

class RingBuffer{
public:
    RingBuffer(uint32_t slots, uint32_t initial_slot_capacity){
        if (slots == 0 || (slots & (slots - 1)) != 0){
            throw std::invalid_argument("slot must be nonzero power of 2");
        }
    
        buffer_.resize(slots);
        buffer_size_ = slots;
        // initial slot capacity becomes useful for writing directly
        // from beast buffer
        for (std::string& slot: buffer_){
            slot.resize(initial_slot_capacity);
        }
    }

    // Will fill the slot with 
    FORCE_INLINE bool TryWrite(std::string& msg);

    // TODO:
    // FORCE_INLINE void WriteFromBuffer();
    
    FORCE_INLINE std::string* TryRead();
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
    std::vector<std::string> buffer_;
};

// RingBuffer::RingBuffer(uint32_t slots, uint32_t initial_slot_capacity) {
//     if (slots == 0 || (slots & (slots - 1)) != 0){
//         throw std::invalid_argument("slot must be nonzero power of 2");
//     }

//     buffer_.resize(slots);
//     buffer_size_ = slots;
//     // initial slot capacity becomes useful for writing directly
//     // from beast buffer
//     for (std::string& slot: buffer_){
//         slot.resize(initial_slot_capacity);
//     }
// }

bool RingBuffer::TryWrite(std::string& msg) {
    if (producer_.counts - consumer_.published_counts.load(std::memory_order_acquire) >= buffer_size_){
        return false;
    }

    // leverages a bit mask to achieve % operation
    std::string& slot = buffer_[producer_.counts & (buffer_size_ - 1)];
    std::swap(slot, msg);

    producer_.counts++;
    producer_.published_counts.store(producer_.counts, std::memory_order_release);
    return true;
}

std::string* RingBuffer::TryRead(){
    if (consumer_.counts == producer_.published_counts.load(std::memory_order_acquire)){
        return nullptr;
    }

    return &buffer_[consumer_.counts & (buffer_size_ - 1)];
}

void RingBuffer::FinishRead(){
    consumer_.counts++;
    consumer_.published_counts.store(consumer_.counts, std::memory_order_release);
}