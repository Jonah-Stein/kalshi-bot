#include "../orderbook/Orderbook.hpp"
#include "../infra/RingBuffer.hpp"
#include "../kalshi/KalshiWsMessages.hpp"
#include "testscripts.hpp"

#include <vector>
#include <string>
#include <thread>
#include <iostream>
#include <cstdint>

void test_orderbook_updates() {

    Orderbook orderbook(100);

    std::unordered_map<int, int> delta_changes;
    std::vector<KalshiOrderbookDelta> deltas = generateDeltaObjects(1000, 100, -1000, 1000, delta_changes);

    for (KalshiOrderbookDelta delta : deltas){
        orderbook.applyDelta(delta);
    }
    
    std::vector<uint32_t> snapshot = orderbook.getSnapshot();
    bool passed = true;
    for (int i = 0; i < 100; i++){
        if (!delta_changes.count(i) && snapshot[i] != 0){
            passed = false;
            std::cout << std::format("{} : no delta, snapshot detects {}\n", i, snapshot[i]);
            break;
        }
        if (delta_changes[i] != snapshot[i]){
            std::cout << std::format("{}: Actual changes: {}; Detected changes: {}\n", i, delta_changes[i], snapshot[i]);
            passed = false;
            break;
        }
    }

    if (!passed){
        std::cout << "Failed orderbook updates";
    } else {
        std::cout << "Passed orderbook updates";
    }
}