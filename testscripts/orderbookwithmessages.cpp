#include "../orderbook/Orderbook.hpp"
#include "../infra/RingBuffer.hpp"
#include "../kalshi/KalshiMessageParser.hpp"
#include "../helpers/helpers.hpp"
#include "testscripts.hpp"

#include <vector>
#include <string>
#include <thread>
#include <iostream>
#include <fstream>

void test_orderbook_with_messages() {
    KalshiMessageParser parser(2);
    Orderbook orderbook(100);

    std::unordered_map<int, int> delta_changes;
    std::vector<std::string> messages = generateDeltaMessages(20000, 100, -1000, 1000, delta_changes);

    RingBuffer ring(1024, 1024);

    std::atomic<bool> all_written = false;

    auto writeToRingBuffer = [&ring, &messages, &all_written](){
        for (std::string msg : messages){
            // this while loop is crucial. Ensures that the message
            // actually gets written
            while (!ring.TryWrite(msg)){
                continue;
            }
        }
        all_written.store(true, std::memory_order_release);
    };
 
    auto consume = [&ring, &all_written, &parser, &orderbook](){
        while (ring.TryRead() != nullptr || !all_written.load(std::memory_order_acquire)){
            if (ring.TryRead() == nullptr){
                continue;
            }
            simdjson::dom::element doc = parser.parseResponse(*ring.TryRead());
            ring.FinishRead();

            KalshiOrderbookDelta delta = parser.fillKalshiOrderbookDelta(doc);
            // apply the delta
            orderbook.applyDelta(delta);
        }
    };
 
    std::thread producer_thread = std::thread(writeToRingBuffer);
    std::thread consumer_thread = std::thread(consume);

    producer_thread.join();
    consumer_thread.join();

    std::vector<uint32_t> snapshot = orderbook.getSnapshot();
    
    bool passed = true;
    int failed = 0;
    for (int i = 0; i < 100; i++){
        if (!delta_changes.count(i) && snapshot[i] != 0){
            passed = false;
            std::cout << std::format("{} : no delta, snapshot detects {}\n", i, snapshot[i]);
            failed++;
        }
        if (delta_changes[i] != snapshot[i]){
            std::cout << std::format("{}: Actual changes: {}; Detected changes: {}\n", i, delta_changes[i], snapshot[i]);
            passed = false;
            failed++;
        }
    }

    std::cout<<std::format("Failed {} / 100\n", failed);
    if (!passed){
        std::cout << "Failed orderbook updates";
    } else {
        std::cout << "Passed orderbook updates";
    }
}

void test_orderbook_with_messages_from_file(const std::string& path) {
    KalshiMessageParser parser(2);
    Orderbook orderbook(100);

    std::ifstream file(path);

    RingBuffer ring(1024, 1024);

    std::atomic<bool> all_written = false;

    std::vector<uint64_t> write_timings;
    auto writeToRingBuffer = [&ring, &file, &all_written, &write_timings](){
        std::string line;
        while (std::getline(file, line)){
            // this while loop is crucial. Ensures that the message
            // actually gets written
            while (!ring.TryWriteWithTiming(line, write_timings)){
                continue;
            }
        }
        all_written.store(true, std::memory_order_release);
    };
    std::vector<uint64_t> seen_timings;
    std::vector<uint64_t> read_timings; 
    auto consume = [&ring, &all_written, &parser, &orderbook, &read_timings, &seen_timings](){
        while (ring.TryRead() != nullptr || !all_written.load(std::memory_order_acquire)){
            if (ring.TryRead() == nullptr){
                continue;
            }
            seen_timings.push_back(timestampNs());
            simdjson::dom::element doc = parser.parseResponse(*ring.TryRead());
            ring.FinishRead();

            KalshiOrderbookDelta delta = parser.fillKalshiOrderbookDelta(doc);
            // apply the delta
            orderbook.applyDelta(delta);
            read_timings.push_back(timestampNs());
        }
    };
 
    std::thread producer_thread = std::thread(writeToRingBuffer);
    std::thread consumer_thread = std::thread(consume);

    producer_thread.join();
    consumer_thread.join();

    std::vector<uint32_t> snapshot = orderbook.getSnapshot();
    // should add some orderbook validation here

    // produce timing stuff:
    LatencyStats write_to_read = calculate_latency_stats(write_timings, read_timings, 0);
    LatencyStats seen_to_read = calculate_latency_stats(seen_timings, read_timings, 0);
    LatencyStats write_to_seen = calculate_latency_stats(write_timings, seen_timings, 0);
    std::cout << "Write to read: \n";
    print_latency_stats(write_to_read);
    std::cout <<"\n\n Seen to read: \n";
    print_latency_stats(seen_to_read);
    std::cout << "Write to seen: \n";
    print_latency_stats(write_to_seen);
}