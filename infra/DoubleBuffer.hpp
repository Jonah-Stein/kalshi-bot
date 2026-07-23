#pragma once
// designed for one writer, one consumer

#include <atomic>
#include <cstddef>


template <typename T> class DoubleBuffer{
public:
    DoubleBuffer() = default;

    T& write_slot(){
        write_index_ = !published_index_.load(std::memory_order_relaxed);
        return slots_[static_cast<std::size_t>(write_index_)];
    }

    void publish() {
        published_index_.store(write_index_, std::memory_order_release);
    }

    T read() const {
        const bool published = published_index_.load(std::memory_order_acquire);
        return slots_[static_cast<std::size_t>(published)];
    }
private:
    T slots_[2]{};
    std::atomic<bool> published_index_{false};
    bool write_index_{true};
};