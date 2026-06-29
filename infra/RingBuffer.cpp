#include "RingBuffer.hpp"
#include <string>
#include <cstdint>
#include <stdexcept>

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

// bool RingBuffer::TryWrite(std::string& msg) {
//     if (producer_.counts - consumer_.published_counts.load(std::memory_order_acquire) >= buffer_size_){
//         return false;
//     }

//     // leverages a bit mask to achieve % operation
//     std::string& slot = buffer_[producer_.counts & (buffer_size_ - 1)];
//     std::swap(slot, msg);

//     producer_.counts++;
//     producer_.published_counts.store(producer_.counts, std::memory_order_release);
//     return true;
// }

// std::string* RingBuffer::TryRead(){
//     if (consumer_.counts == producer_.published_counts.load(std::memory_order_acquire)){
//         return nullptr;
//     }

//     return &buffer_[consumer_.counts & (buffer_size_ - 1)];
// }

// void RingBuffer::FinishRead(){
//     consumer_.counts++;
//     consumer_.published_counts.store(consumer_.counts, std::memory_order_release);
// }