#pragma once
#include <cstdint>

void runRingBufferTests();
bool testRingBuffer(uint32_t message_count, uint64_t& pcycle_counts, uint64_t& ccycle_counts);