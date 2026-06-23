#include <atomic>
#include <algorithm>
#include <string>
#include <cstdint>
#include <vector>

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
    RingBuffer(uint32_t slots, uint32_t starting_slot_capacity);

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