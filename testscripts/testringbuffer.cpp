#include "testscripts.hpp"
#include "../infra/RingBuffer.hpp"
#include <thread>
#include <atomic>
#include <iostream>
#include <format>
#include <unordered_map>
#include <algorithm>
#include <cstdint>

void runRingBufferTests(){
    int messages;
    int tests;
    std::cout << "How many messages to send?: ";
    std::cin >> messages;
    std::cout <<"How many tests?: ";
    std::cin >> tests;
    // int messages = 1000;
    int passed = 0;

    uint64_t pcycles = 0;
    uint64_t ccycles = 0;
    for (int i = 0; i < tests; i++){
        if (testRingBuffer(messages, pcycles, ccycles)){
            passed++;
        }
    }
    double avg_c_cycle = ccycles/tests;
    double avg_p_cycle = pcycles/tests;
    std::cout<< passed << "/" << tests<<" passed\n";
    std::cout<<std::format("Avg consumer cycles: {}\n Avg producer cycles {}\n", avg_c_cycle, avg_p_cycle);
}

bool testRingBuffer(uint32_t message_count, uint64_t& pcycle_counts, uint64_t& ccycle_counts){
    uint32_t buffer_size = 1024;
    uint32_t buffer_slot_size = 1024;
    RingBuffer ring(buffer_size, buffer_slot_size);

    std::unordered_map<std::string, int> produced;
    std::unordered_map<std::string, int> consumed;
    std::atomic<bool> done{false};
    uint64_t total_producer_cycles= 0;
    uint64_t total_consumer_cycles = 0;

    auto produce = [&ring, buffer_size, &produced, &done, &pcycle_counts](uint32_t messages){
        uint32_t i = 0;
        char c = static_cast<char>(65);
        int charcounter = 0;
        // go til 90 and then loop around
        while (i < messages){
            uint32_t boundary = std::min(i + buffer_size, messages);
            for (uint32_t j = i; j < boundary; j++){
                pcycle_counts++;
                std::string msg(1, c);
                produced[msg]++;
                bool written = ring.TryWrite(msg);
                while (!written){
                    written = ring.TryWrite(msg);
                }
            }
            i = boundary;
            charcounter++;
            c = static_cast<char>(65 + (charcounter% 26));
        }
        // std::cout<<std::format("Producer cycles: {}\n", cycles);
        done.store(true, std::memory_order_release);
    };

    auto consume = [&ring, &consumed, &done, &ccycle_counts](){
        while (!done.load(std::memory_order_acquire) || !(ring.TryRead() == nullptr)){
            ccycle_counts++;
            std::string* s = ring.TryRead();
            if (s != nullptr){
                consumed[*s]++;
                ring.FinishRead();
            }
        }
        // std::cout<<std::format("Consumer cycles: {}\n", cycles);
    };
    
    std::thread producer_thread = std::thread(produce, message_count);
    std::thread consumer_thread = std::thread(consume);

    // Need to check how we can know when the jobs are done running
    // Might be able to have atomic checks

    producer_thread.join();
    consumer_thread.join();
    bool passed = true;
    for (const auto& [key, value] : produced){
        // std::cout << std::format("{}: {} produced, {} consumed\n", key, value, consumed[key]);
        if (value != consumed[key]){
            // std::cout<<"FAILED: ";
            passed = false;
        }
    }

    if (passed){
        // std::cout << "Test passed!";
        return true;
    } else {
        std::cout << "Tests failed";
        return false;
    }
}