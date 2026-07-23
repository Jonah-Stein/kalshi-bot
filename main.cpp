#include "kalshi/KalshiAuth.hpp"
#include "kalshi/KalshiRestClient.hpp"
#include "kalshi/KalshiWsClient.hpp"
#include "testscripts/testscripts.hpp"
#include "kalshi/KalshiMessageParser.hpp"
#include "orderbook/Orderbook.hpp"
#include "infra/StringRingBuffer.hpp"
#include "infra/DoubleBuffer.hpp"
#include "api/types.hpp"
#include "api/MetricsHttpServer.hpp"
#include "helpers/helpers.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <stdexcept>
#include <queue>
#include <thread>
#include <format>
#include <chrono>
#include <iostream>
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

void runSystem(const std::string& market_ticker, int decimal_degrees, KalshiWsClient& ws_client, KalshiRestClient& rest_client){
    // in future, this should take into account decimal_degrees 
    Orderbook orderbook(100);
    KalshiMessageParser parser(2);

    // FOR BENCHMARKING:
    std::vector<uint64_t> times_finished;
    times_finished.reserve(1024);


    StringRingBuffer ring(1024, 1024);
    int num_received = 0;
    auto writeToRingBuffer = [&ring, &num_received](std::string& msg){
        while (!ring.TryWrite(msg)){
            continue;
        };
        num_received++;
    };

    auto consume = [&ring, &parser, &orderbook, &times_finished](){
        bool stopped = false;
        std::string_view type;
        // simdjson::ondemand::object msg;

        while (!stopped){
            if (ring.TryRead() == nullptr){
                continue;
            }

            // need to change this
            simdjson::dom::element doc = parser.parseResponse(*ring.TryRead());
            // std::cout<<*ring.TryRead()<<"\n";
            ring.FinishRead();

            doc["type"].get_string().get(type);

            // can do this efficiently.
            // Need to add a stop message to th ring buffer
            // choosing between 'orderbook_delta', 'orderbook_snapshot', 'stop', 'subscribed'
            // std::cout<< "type: " << type<< "\n";
            switch (type.size()){
                case 15: {
                    // delta
                    KalshiOrderbookDelta delta = parser.fillKalshiOrderbookDelta(doc);
                    orderbook.applyDelta(delta);
                    break;
                }
                case 18: {
                    //snapshot
                    KalshiOrderbookSnapshot snapshot = parser.fillKalshiOrderbookSnapshot(doc);
                    orderbook.applySnapshot(snapshot);
                    break;
                }
                case 4: {
                    //stop
                    stopped = true;
                    break;
                }
            }
            times_finished.push_back(timestampNs());
        }
    };

    ws_client.start(writeToRingBuffer, market_ticker);
    std::thread consumer_thread = std::thread(consume);

    // TODO: Need some sign handler to stop the websocket
    std::this_thread::sleep_for(std::chrono::seconds(20));
    ws_client.stop();
    consumer_thread.join();


    // get market orderbook from rest api, create a new orderbook
    // then compare the two orderbooks
    std::string current_snapshot_res = rest_client.getMarketOrderbook(market_ticker);

    simdjson::dom::element doc = parser.parseResponse(current_snapshot_res);
    KalshiOrderbookSnapshot snapshot = parser.fillKalshiOrderbookSnapshotFromRest(doc);

    Orderbook current(100);
    current.applySnapshot(snapshot);

    // compare the two orderbooks
    std::vector<uint32_t> calculated_snapshot = orderbook.getSnapshot();
    std::vector<uint32_t> current_snapshot = current.getSnapshot();

    int differences = 0;
    for (int i = 0; i < 100; i++){
        if (calculated_snapshot[i] == current_snapshot[i]){
            continue;
        }
        differences++;
        std::cout<<std::format("{}: Calculated: {} ; Current: {}\n", i, calculated_snapshot[i], current_snapshot[i]);
    }
    std::cout << differences << " differences\n";
    std::cout << "Received " << num_received << " messages\n";

    // std::cout<<"\nTiming differences: \n";

    std::vector<uint64_t> times_received = ws_client.getMessageArrivalTimes();
    uint64_t overall_diff = 0;
    for (int i = 2; i < times_received.size(); i++){
        uint64_t diff =times_finished[i] - times_received[i];
        overall_diff += diff;
        std::cout << diff << " ns\n";
    }
    std::cout << "Average processing time: " << (overall_diff / (times_received.size()-2)) << "\n";
}


int main(){
    DoubleBuffer<OrderbookMetrics> metrics_buf;

    std::uint16_t metrics_port = 8080;
    if (const char* port_env = std::getenv("METRICS_HTTP_PORT")) {
        metrics_port = static_cast<std::uint16_t>(std::stoi(port_env));
    }

    MetricsHttpServer metrics_server(metrics_buf, "0.0.0.0", metrics_port);
    metrics_server.start();

    // Sample publish so GET /metrics works before the trading pipeline is wired.
    // TODO: call write_slot()/publish() from the orderbook consumer every N messages after applyDelta.
    {
        OrderbookMetrics& slot = metrics_buf.write_slot();
        slot.messages_received = 42;
        slot.bid_price = 45;
        slot.bid_quantity = 100;
        slot.ask_price = 46;
        slot.ask_quantity = 80;
        metrics_buf.publish();
    }

    if (std::getenv("METRICS_SMOKE")) {
        std::cout << "METRICS_SMOKE set: serving metrics on port " << metrics_port
                  << " (Ctrl+C to exit)\n";
        while (true) {
            std::this_thread::sleep_for(std::chrono::hours(24));
        }
    }

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

    std::string sampleSnapshot = R"({"type":"orderbook_snapshot","sid":1,"seq":1,"msg":{"market_ticker":"KXHIGHNY-26JUN24-T82","market_id":"0445cdcb-e22f-4522-9981-aed4b66015ff","yes_dollars_fp":[["0.0100","2230.99"],["0.0200","806.00"],["0.0300","156.76"],["0.0400","101.00"],["0.0500","35.30"],["0.0600","2267.00"],["0.0700","151.00"],["0.0800","248.24"],["0.0900","81.00"],["0.1000","29.00"]],"no_dollars_fp":[["0.0100","419.78"],["0.0200","397.00"],["0.0300","200.00"],["0.0400","100.00"],["0.0600","100.00"],["0.0800","0.92"],["0.0900","20.32"],["0.1000","55.00"],["0.1100","100.00"],["0.1300","50.00"],["0.1500","5.00"],["0.1600","16.00"],["0.1800","123.42"],["0.2000","5.00"],["0.2400","11.96"],["0.2500","95.35"],["0.2600","100.00"],["0.2700","1612.34"],["0.2800","10.00"],["0.3000","127.00"],["0.3100","22.00"],["0.3200","41.19"],["0.3300","400.00"],["0.3400","22.00"],["0.3500","127.00"],["0.3600","18.00"],["0.3700","39.10"],["0.3800","19.96"],["0.3900","12.00"],["0.4000","25.00"],["0.4400","25.00"],["0.4500","5.00"],["0.4800","16.67"],["0.4900","2.00"],["0.5000","56.19"],["0.5500","105.00"],["0.5800","7.00"],["0.6000","6.00"],["0.6100","0.16"],["0.6200","15.00"],["0.6300","8.00"],["0.6400","22.00"],["0.6500","5.00"],["0.6800","88.00"],["0.6900","4.00"],["0.7000","5.00"],["0.7200","0.05"],["0.7500","5.00"],["0.7800","142.60"],["0.7900","1.46"],["0.8000","49.30"],["0.8100","677.00"],["0.8300","471.00"],["0.8400","248.28"],["0.8500","206.00"],["0.8800","78.00"],["0.8900","207.00"]]}})";

    KalshiWsClient ws_client(auth, ws_url, ws_connection_path);
    // runSystem("KXWCADVANCE-26JUL11NORENG-NOR", 2, ws_client, rest_client);
    // "KXWCADVANCE-26JUL11NORENG-NOR"
    std::unordered_map<int, int> delta_changes;
    // generateDeltasFile(1000000, 100, -1000, 1000, delta_changes, "generated_deltas.jsonl");
    // test_or_orderbook_with_messages_from_file("generated_deltas.jsonl");
    uint64_t start = timestampNs();
    raw_test_or_orderbook_with_messages_from_file("generated_deltas.jsonl");
    uint64_t end = timestampNs();
    std::cout << "Object ring took: " << end-start << " ns\n";
    
    start = timestampNs();
    raw_test_orderbook_with_messages_from_file("generated_deltas.jsonl");
    end = timestampNs();
    std::cout << "String ring took: " << end-start << " ns\n";

    start = timestampNs();
    raw_test_or_orderbook_with_messages_from_file("generated_deltas.jsonl");
    end = timestampNs();
    std::cout << "Object ring took: " << end-start << " ns\n";

    start = timestampNs();
    raw_test_orderbook_with_messages_from_file("generated_deltas.jsonl");
    end = timestampNs();
    std::cout << "String ring took: " << end-start << " ns\n";
    // testwebsocket(ws_client);

}