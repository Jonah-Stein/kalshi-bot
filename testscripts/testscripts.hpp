#pragma once
#include "../kalshi/KalshiWsMessages.hpp"
#include "../kalshi/KalshiWsClient.hpp"
#include "../orderbook/Orderbook.hpp"
#include <cstdint>
#include <vector>
#include <string>
#include <map>

void runRingBufferTests();
bool testRingBuffer(uint32_t message_count, uint64_t& pcycle_counts, uint64_t& ccycle_counts);


std::vector<std::string> generateDeltaMessages(int num_messages, 
    int price_denominations,
    int quantity_lb,
    int quantity_ub,
    std::unordered_map<int, int>& cumulated_quantities);

std::vector<KalshiOrderbookDelta> generateDeltaObjects(int num_messages, 
        int price_denominations,
        int quantity_lb,
        int quantity_ub,
        std::unordered_map<int, int>& cumulated_quantities);

std::string generateDeltaMessage(int seq, int price, int quantity, KalshiSide side, uint64_t ts_ms, int price_denominations);
KalshiOrderbookDelta generateDeltaObject(int price, int quantity, KalshiSide side, uint64_t ts_ms);
uint64_t timestampMs();
void generateDeltasFile(int num_messages, 
    int price_denominations,
    int quantity_lb,
    int quantity_ub,
    std::unordered_map<int, int>& cumulated_quantities, const std::string& file_name);


void testwebsocket(KalshiWsClient& ws_client);

void test_orderbook_with_messages();
void test_orderbook_with_messages_from_file(const std::string& path);
void test_or_orderbook_with_messages_from_file(const std::string& path);

void test_orderbook_updates();