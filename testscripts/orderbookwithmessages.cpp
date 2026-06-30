#include "../orderbook/Orderbook.hpp"
#include "../infra/RingBuffer.hpp"
#include "../kalshi/KalshiMessageParser.hpp"
#include "testscripts.hpp"

#include <vector>
#include <string>
#include <thread>
#include <iostream>

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

    // Need to edit this consume to actually update the order
    // and this will probably be the actual production logic
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