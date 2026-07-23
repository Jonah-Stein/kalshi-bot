#include "../orderbook/Orderbook.hpp"
#include "../infra/StringRingBuffer.hpp"
#include "../infra/ObjectRingBuffer.hpp"
#include "../kalshi/KalshiMessageParser.hpp"
#include "../helpers/helpers.hpp"
#include "testscripts.hpp"

#include <vector>
#include <string>
#include <thread>
#include <iostream>
#include <fstream>

void raw_test_orderbook_with_messages_from_file(const std::string& path) {
    KalshiMessageParser parser(2);
    Orderbook orderbook(100);

    std::ifstream file(path);

    StringRingBuffer ring(1024, 1024);

    std::atomic<bool> all_written = false;

    auto writeToRingBuffer = [&ring, &file, &all_written](){
        std::string line;
        while (std::getline(file, line)){
            // this while loop is crucial. Ensures that the message
            // actually gets written
            while (!ring.TryWrite(line)){
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
            // simulating strategy time
            rest_ns(800);
        }
    };
 
    std::thread producer_thread = std::thread(writeToRingBuffer);
    std::thread consumer_thread = std::thread(consume);

    producer_thread.join();
    consumer_thread.join();
}

// object ring
void raw_test_or_orderbook_with_messages_from_file(const std::string& path) {
    KalshiMessageParser parser(2);
    Orderbook orderbook(100);

    std::ifstream file(path);

    ObjectRingBuffer ring(1024);

    std::atomic<bool> all_written = false;

    auto writeToRingBuffer = [&ring, &file, &all_written, &parser](){
        std::string line;
        while (std::getline(file, line)){

            // this while loop is crucial. Ensures that the message
            // actually gets written
            // parse line first
            simdjson::dom::element doc = parser.parseResponse(line);
            KalshiOrderbookDelta delta = parser.fillKalshiOrderbookDelta(doc);

            while (!ring.TryWriteDelta(delta.price, delta.quantity_hundredths, delta.side, delta.ts_ms)){
                continue;
            }
        }
        all_written.store(true, std::memory_order_release);
    };

    auto consume = [&ring, &all_written, &orderbook](){
        while (ring.TryRead() != nullptr || !all_written.load(std::memory_order_acquire)){
            if (ring.TryRead() == nullptr){
                continue;
            }
            // apply the delta
            orderbook.applyDelta((*ring.TryRead()).delta);
            ring.FinishRead();
            
            // simulating strategy time
            rest_ns(800);
        }
    };
 
    std::thread producer_thread = std::thread(writeToRingBuffer);
    std::thread consumer_thread = std::thread(consume);

    producer_thread.join();
    consumer_thread.join();
}

void raw_orderbook_with_messages_from_file_time_trials(const std::string& path){
    uint64_t start = timestampNs();
    raw_test_or_orderbook_with_messages_from_file(path);
    uint64_t end = timestampNs();
    std::cout << "Object ring took: " << end-start << " ns\n";
    
    start = timestampNs();
    raw_test_orderbook_with_messages_from_file(path);
    end = timestampNs();
    std::cout << "String ring took: " << end-start << " ns\n";

    start = timestampNs();
    raw_test_or_orderbook_with_messages_from_file(path);
    end = timestampNs();
    std::cout << "Object ring took: " << end-start << " ns\n";

    start = timestampNs();
    raw_test_orderbook_with_messages_from_file(path);
    end = timestampNs();
    std::cout << "String ring took: " << end-start << " ns\n";
}