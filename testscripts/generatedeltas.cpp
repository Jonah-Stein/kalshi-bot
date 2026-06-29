#include "testscripts.hpp"
#include "../kalshi/KalshiWsMessages.hpp"
#include <string>
#include <vector>
#include <random>
#include <cstdint>
#include <chrono>
#include <map>
#include <format>


std::string generateDeltaMessage(int seq, int price, int quantity, KalshiSide side, uint64_t ts_ms){
    std::string s = side == KalshiSide::Yes ? "yes" : "no";
    //convert price to string
    std::string price_str = std::format("0.{:-04}", price);
    //convert quantity to string
    std::string quantity_str = std::format("{}.00", quantity);

    return std::format(R"({{"type":"orderbook_delta","sid":1,"seq":{},"msg":{{"market_ticker":"KXHIGHNY-26JUN24-T82","market_id":"0445cdcb-e22f-4522-9981-aed4b66015ff","price_dollars":"{}","delta_fp":"{}","side":"{}","ts":"2026-06-24T15:49:55.033836Z","ts_ms":{}}}}})"
        , seq, price_str, quantity_str, s, ts_ms);
    
}

// cumulated quantities is used to test against the orderbook and also generate
// correct deltas that don't push quantities negative
std::vector<std::string> generateDeltaMessages(int num_messages, 
                                        int price_denominations,
                                        int quantity_lb,
                                        int quantity_ub,
                                        std::unordered_map<int, int>& cumulated_quantities){
    std::vector<std::string> deltas;


    // some random generator
    static std::default_random_engine engine{std::random_device{}()};
    // is this inclusive?
    std::uniform_int_distribution<int> randomPrice{1, price_denominations-1};
    std::uniform_int_distribution<int> randomQuantity{quantity_lb, quantity_ub};
    std::uniform_int_distribution<int> chooseSide{0,1};

    for (int i = 0; i < num_messages; i++){
        // generate price
        int price = randomPrice(engine);

        // generate quantity
        if (!cumulated_quantities.count(price)){
            cumulated_quantities[price] = 0;
        }
        int quantity = randomQuantity(engine);
        while (quantity < 0 && -quantity > cumulated_quantities[price]){
            quantity = randomQuantity(engine);
        }
        cumulated_quantities[price] += quantity;
        // pick side
        KalshiSide side = chooseSide(engine) == 0 ? KalshiSide::Yes : KalshiSide::No;

        uint64_t ts_ms = timestampMs();
        // generate
        deltas.push_back(generateDeltaMessage(i+1, price, quantity, side, ts_ms));
    }
    return deltas;
}

KalshiOrderbookDelta generateDeltaObject(int price, int quantity, KalshiSide side, uint64_t ts_ms){
    KalshiOrderbookDelta delta;
    delta.price = price;
    delta.quantity_hundredths = quantity;
    delta.side = side;
    delta.ts_ms = ts_ms;

    return delta;
}

std::vector<KalshiOrderbookDelta> generateDeltaObjects(int num_messages, 
    int price_denominations,
    int quantity_lb,
    int quantity_ub,
    std::unordered_map<int, int>& cumulated_quantities){
    std::vector<KalshiOrderbookDelta> deltas;

    // some random generator
    static std::default_random_engine engine{std::random_device{}()};
    // is this inclusive?
    std::uniform_int_distribution<int> randomPrice{1, price_denominations-1};
    std::uniform_int_distribution<int> randomQuantity{quantity_lb, quantity_ub};
    std::uniform_int_distribution<int> chooseSide{0,1};

    for (int i = 0; i < num_messages; i++){
        // generate price
        int price = randomPrice(engine);

        // generate quantity
        if (!cumulated_quantities.count(price)){
            cumulated_quantities[price] = 0;
        }
        int quantity = randomQuantity(engine);
        while (quantity < 0 && -quantity > cumulated_quantities[price]){
            quantity = randomQuantity(engine);
        }
        cumulated_quantities[price] += quantity;
        // pick side
        KalshiSide side = chooseSide(engine) == 0 ? KalshiSide::Yes : KalshiSide::No;

        uint64_t ts_ms = timestampMs();
        // generate
        deltas.push_back(generateDeltaObject(price, quantity, side, ts_ms));
    }
    return deltas;
}






uint64_t timestampMs(){
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}