#include "KalshiMessageParser.hpp"
#include "KalshiWsMessages.hpp"

#include <format>
#include <stdexcept>
#include <simdjson.h>


simdjson::dom::element KalshiMessageParser::parseResponse(std::string& res){
    simdjson::dom::element doc;
    auto err = json_parser_.parse(res).get(doc);
    if (err) {
        throw std::runtime_error(
            std::format("JSON parse error: {}", simdjson::error_message(err))
        );
    }
    return doc;
}

void KalshiMessageParser::changePriceDecimalDegree(uint8_t new_degree){
    price_dec_degree_ = new_degree;
    // should add in a check to make sure this is <= 4
}


KalshiOrderbookDelta KalshiMessageParser::fillKalshiOrderbookDelta(simdjson::dom::element& doc){
    simdjson::dom::element msg = doc["msg"];
    KalshiOrderbookDelta delta;

    // fill price
    std::string_view price_str = msg["price_dollars"];
    for (int i = 2; i < 2 + price_dec_degree_; i++){
        delta.price *= 10;
        delta.price += static_cast<uint16_t>(price_str[i] - '0');
    }

    // fill quantity
    std::string_view delta_str = msg["delta_fp"];
    int i = 0;
    if (delta_str[0] == '-'){
        delta.quantity_hundredths = -1;
        i = 1;
    }
    for (; i < delta_str.size(); i++){
        if (delta_str[i] == '.'){
            continue;
        }
        delta.quantity_hundredths *= 10;
        delta.quantity_hundredths += static_cast<uint16_t>(delta_str[i] - '0');
    }

    // fill side
    std::string_view side_str = msg["side"];
    if (side_str[0] == 'n'){
        delta.side = KalshiSide::No;
    } else {
        delta.side = KalshiSide::Yes;
    }

    // fill ts_ms
    std::string_view ts_str = msg["ts_ms"];
    for (int i = 0; i < ts_str.size(); i++){
        delta.ts_ms *= 10;
        delta.ts_ms += static_cast<uint64_t>(ts_str[i] - '0');
    }

    return delta;
}
KalshiOrderbookSnapshot KalshiMessageParser::fillKalshiOrderbookSnapshot(simdjson::dom::element& doc){
    // simdjson::dom::element* mes = doc["msg"];
    KalshiOrderbookSnapshot snapshot;


    return snapshot;
}