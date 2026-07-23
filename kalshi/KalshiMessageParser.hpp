#pragma once

#include "KalshiWsMessages.hpp"
#include <simdjson.h>


class KalshiMessageParser {
public:
    KalshiMessageParser(uint8_t price_dec_degree): price_dec_degree_(price_dec_degree){};
    void changePriceDecimalDegree(uint8_t new_degree);

    simdjson::dom::element parseResponse(std::string& res);
    // simdjson::ondemand::document parseResponse(simdjson::padded_string& res);

    KalshiOrderbookDelta fillKalshiOrderbookDelta(simdjson::dom::element& doc);
    KalshiOrderbookSnapshot fillKalshiOrderbookSnapshot(simdjson::dom::element& doc);
    KalshiOrderbookSnapshot fillKalshiOrderbookSnapshotFromRest(simdjson::dom::element& doc);

    void fillKalshiOrderbookDelta(simdjson::dom::element& doc, KalshiOrderbookDelta& delta);
    void fillKalshiOrderbookSnapshot(simdjson::dom::element& doc, KalshiOrderbookSnapshot& snapshot);
    // KalshiOrderbookDelta fillKalshiOrderbookDelta(simdjson::ondemand::object& msg);
    // KalshiOrderbookSnapshot fillKalshiOrderbookSnapshot(simdjson::ondemand::object& msg);
    // KalshiOrderbookSnapshot fillKalshiOrderbookSnapshotFromRest(simdjson::ondemand::object& msg);
private:
    simdjson::dom::parser json_parser_;
    uint8_t price_dec_degree_;

    uint16_t parsePrice(const std::string_view& str);
};