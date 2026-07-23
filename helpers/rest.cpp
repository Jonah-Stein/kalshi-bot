#include <chrono>
#include <thread>
#include <cstdint>

void rest_ns(uint64_t ns){
    using namespace std::chrono;
    const auto deadline = steady_clock::now() + nanoseconds(ns);
    while (steady_clock::now() < deadline){
    }
}