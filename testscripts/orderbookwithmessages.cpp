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

    std::unordered_map<int, int> delta_changes;
    std::vector<std::string> messages = generateDeltaMessages(1000, 100, -1000, 1000, delta_changes);

    RingBuffer ring(1024, 1024);

    std::atomic<bool> all_written = false;

    auto writeToRingBuffer = [&ring, &messages, &all_written](){
        for (std::string msg : messages){
            ring.TryWrite(msg);
        }
        all_written.store(false, std::memory_order_release);
    };

    // Need to edit this consume to actually update the order
    // and this will probably be the actual production logic
    auto consume = [&ring, &all_written, &parser](){
        while (ring.TryRead() != nullptr || !all_written.load(std::memory_order_acquire)){
            simdjson::dom::element doc = parser.parseResponse(*ring.TryRead());

            KalshiOrderbookDelta d = parser.fillKalshiOrderbookDelta(doc);

            // apply the delta
            std::cout<< *ring.TryRead()<<"\n";
            ring.FinishRead();
        }
    };
 
    std::thread producer_thread = std::thread(writeToRingBuffer);
    std::thread consumer_thread = std::thread(consume);

    producer_thread.join();
    consumer_thread.join();


    // then need to validate results.
    std::cout <<"Done\n";
}