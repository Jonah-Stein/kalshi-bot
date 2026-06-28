#include "kalshi/KalshiAuth.hpp"
#include "kalshi/KalshiRestClient.hpp"
#include "kalshi/KalshiWsClient.hpp"
#include "testscripts/testringbuffer.hpp"
#include "kalshi/KalshiMessageParser.hpp"
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

    std::string sampleDelta = R"({"type":"orderbook_delta","sid":1,"seq":2,"msg":{"market_ticker":"KXHIGHNY-26JUN24-T82","market_id":"0445cdcb-e22f-4522-9981-aed4b66015ff","price_dollars":"0.5800","delta_fp":"-7.00","side":"no","ts":"2026-06-24T15:49:55.033836Z","ts_ms":1782316195033}})";
    KalshiMessageParser parser(2);
    simdjson::dom::element doc = parser.parseResponse(sampleDelta);

    std::cout<<doc["msg"]["price_dollars"]<<"\n";

    std::string_view price = doc["msg"]["price_dollars"];
    // looked into it, and supposedly its fastest to manually construt the
    // price by iterating though the substring
    int degree =2;
    uint16_t p = 0;
    for (int i = 2; i < 2+degree; i++){
        p *= 10;
        p += static_cast<int>(price[i] - '0');
    }
    std::cout<< "The final result is: " << p << "\n";

    //doc[msg].. is a dom::element type. Need to find a way to efficiently convert this
    // to an int (without having to go from this -> string -> int)

    // std::queue<std::string> q;
    // auto readIntoQueue = [&q](const std::string& msg){

    // }
    // KalshiWsClient ws_client(auth, ws_url, ws_connection_path);


    // auto printOutputs = [](const std::string& msg){
    //     std::cout<<msg<<"\n";
    // };

    // ws_client.start(printOutputs, "KXHIGHNY-26JUN24-T82");
    // std::this_thread::sleep_for(std::chrono::seconds(10));
    // std::cout<<"Stopping...\n";
    
    // ws_client.stop();

}