#include "kalshi/KalshiAuth.hpp"
#include "kalshi/KalshiRestClient.hpp"
#include "kalshi/KalshiWsClient.hpp"
#include "testscripts/testscripts.hpp"
#include "kalshi/KalshiMessageParser.hpp"
#include "orderbook/Orderbook.hpp"
#include "infra/RingBuffer.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <stdexcept>
#include <queue>
#include <format>
#include <chrono>
#include <simdjson.h>

std::string readFile(const std::string& path){
    // input file stream
    std::ifstream file(path);
    if (!file){
        throw std::runtime_error("Couldn't open file");
    }

    // output string stream
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string readEnvVar(const std::string& var_name){
    const char* raw_var = std::getenv(var_name.c_str());
    if (!raw_var){
        throw std::runtime_error(std::format("{} not set", var_name));
    }
    return raw_var;
}

// have to make something very similar to this, but instead of taking a ws_client,
// it takes in a vector of strings


int main(){
    std::string base_url = readEnvVar("KALSHI_API_BASE_URL");
    std::string api_key = readEnvVar("KALSHI_API_KEY");
    std::string pem_path = readEnvVar("KALSHI_PEM_PATH");
    std::string ws_url = readEnvVar("KALSHI_WS_URL");
    std::string ws_connection_path = readEnvVar("KALSHI_WS_CONNECTION_PATH");

    // Have to load pem file
    std::string pem = readFile(pem_path);

    KalshiAuth auth(pem, api_key);
    KalshiRestClient rest_client(auth, base_url);
    // rest_client.printSeriesInfo("KXHIGHNY");
    // rest_client.printMarketsBySeries("KXHIGHNY");

    std::string sampleDelta = R"({"type":"orderbook_delta","sid":1,"seq":2,"msg":{"market_ticker":"KXHIGHNY-26JUN24-T82","market_id":"0445cdcb-e22f-4522-9981-aed4b66015ff","price_dollars":"0.1000","delta_fp":"7.00","side":"yes","ts":"2026-06-24T15:49:55.033836Z","ts_ms":1782316195033}})";
    KalshiMessageParser parser(2);
    simdjson::dom::element doc = parser.parseResponse(sampleDelta);

    // std::cout<<doc["msg"]["price_dollars"]<<"\n";

    // std::string_view price = doc["msg"]["price_dollars"];
    // // looked into it, and supposedly its fastest to manually construt the
    // // price by iterating though the substring
    // int degree =2;
    // uint16_t p = 0;
    // for (int i = 2; i < 2+degree; i++){
    //     p *= 10;
    //     p += static_cast<int>(price[i] - '0');
    // }
    // std::cout<< "The final result is: " << p << "\n";

    // // testing the parser
    KalshiOrderbookDelta d = parser.fillKalshiOrderbookDelta(doc);
    // std::cout << std::format("Price: {}\n", d.price);

    std::string sampleSnapshot = R"({"type":"orderbook_snapshot","sid":1,"seq":1,"msg":{"market_ticker":"KXHIGHNY-26JUN24-T82","market_id":"0445cdcb-e22f-4522-9981-aed4b66015ff","yes_dollars_fp":[["0.0100","2230.99"],["0.0200","806.00"],["0.0300","156.76"],["0.0400","101.00"],["0.0500","35.30"],["0.0600","2267.00"],["0.0700","151.00"],["0.0800","248.24"],["0.0900","81.00"],["0.1000","29.00"]],"no_dollars_fp":[["0.0100","419.78"],["0.0200","397.00"],["0.0300","200.00"],["0.0400","100.00"],["0.0600","100.00"],["0.0800","0.92"],["0.0900","20.32"],["0.1000","55.00"],["0.1100","100.00"],["0.1300","50.00"],["0.1500","5.00"],["0.1600","16.00"],["0.1800","123.42"],["0.2000","5.00"],["0.2400","11.96"],["0.2500","95.35"],["0.2600","100.00"],["0.2700","1612.34"],["0.2800","10.00"],["0.3000","127.00"],["0.3100","22.00"],["0.3200","41.19"],["0.3300","400.00"],["0.3400","22.00"],["0.3500","127.00"],["0.3600","18.00"],["0.3700","39.10"],["0.3800","19.96"],["0.3900","12.00"],["0.4000","25.00"],["0.4400","25.00"],["0.4500","5.00"],["0.4800","16.67"],["0.4900","2.00"],["0.5000","56.19"],["0.5500","105.00"],["0.5800","7.00"],["0.6000","6.00"],["0.6100","0.16"],["0.6200","15.00"],["0.6300","8.00"],["0.6400","22.00"],["0.6500","5.00"],["0.6800","88.00"],["0.6900","4.00"],["0.7000","5.00"],["0.7200","0.05"],["0.7500","5.00"],["0.7800","142.60"],["0.7900","1.46"],["0.8000","49.30"],["0.8100","677.00"],["0.8300","471.00"],["0.8400","248.28"],["0.8500","206.00"],["0.8800","78.00"],["0.8900","207.00"]]}})";

    simdjson::dom::element docs = parser.parseResponse(sampleSnapshot);
    // simdjson::dom::element msg = doc["msg"];

    // simdjson::dom::array yes_levels = msg["yes_dollars_fp"].get_array();
    // for (simdjson::dom::element level : yes_levels){
    //     simdjson::dom::array pair = level.get_array();
    //     std::cout<< pair.at(0) << " : " << pair.at(1) << "\n";
    // }
    // std::cout <<"The real snapshot: \n";
    KalshiOrderbookSnapshot s = parser.fillKalshiOrderbookSnapshot(docs);
    // for (KalshiPriceLevel l : s.yes_levels){
    //     std::cout << std::format("{} : {}\n", l.price, l.quantity);
    // }

    Orderbook o(100);
    o.applySnapshot(s);
    o.applyDelta(d);
    // o.printSnapshot();


    // KalshiWsClient ws_client(auth, ws_url, ws_connection_path);
    // testwebsocket(ws_client);



    

}