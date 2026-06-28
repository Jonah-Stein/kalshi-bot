#pragma once

#include "KalshiWsMessages.hpp"
#include <simdjson.h>


class KalshiMessageParser {
public:
    KalshiMessageParser(uint8_t price_dec_degree): price_dec_degree_(price_dec_degree){};
    void changePriceDecimalDegree(uint8_t new_degree);

    simdjson::dom::element parseResponse(std::string& res);

    KalshiOrderbookDelta fillKalshiOrderbookDelta(simdjson::dom::element& doc);
    KalshiOrderbookSnapshot fillKalshiOrderbookSnapshot(simdjson::dom::element& doc);
private:
    simdjson::dom::parser json_parser_;
    uint8_t price_dec_degree_;
};