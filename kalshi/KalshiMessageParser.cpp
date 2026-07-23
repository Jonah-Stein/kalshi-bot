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

uint16_t KalshiMessageParser::parsePrice(const std::string_view& str){
    uint16_t p = 0;
    for (int i = 2; i < 2 + price_dec_degree_; i++){
        p *= 10;
        p += static_cast<int>(str[i] - '0');
    }
    return p;
}


KalshiOrderbookDelta KalshiMessageParser::fillKalshiOrderbookDelta(simdjson::dom::element& doc){
    // simdjson::dom::element msg = doc["msg"];

    // // fill price
    // std::string_view price_str = msg["price_dollars"].get_string();
    // delta.price = parsePrice(price_str); 

    // // fill quantity
    // std::string_view delta_str = msg["delta_fp"].get_string();
    // int i = 0;
    // bool neg_delta = false;
    // delta.quantity_hundredths = 0;
    // if (delta_str[0] == '-'){
    //     neg_delta = true;
    //     i = 1;
    // }
    // for (; i < delta_str.size(); i++){
    //     if (delta_str[i] == '.'){
    //         continue;
    //     }
    //     delta.quantity_hundredths *= 10;
    //     delta.quantity_hundredths += static_cast<uint16_t>(delta_str[i] - '0');
    // }
    // if (neg_delta){
    //     delta.quantity_hundredths *= -1;
    // }

    // // fill side
    // std::string_view side_str = msg["side"].get_string();
    // if (side_str[0] == 'n'){
    //     delta.side = KalshiSide::No;
    // } else {
    //     delta.side = KalshiSide::Yes;
    // }

    // // fill ts_ms
    // // might have to undo this .get_string()
    // delta.ts_ms = msg["ts_ms"].get_uint64();
    KalshiOrderbookDelta delta;
    fillKalshiOrderbookDelta(doc, delta);
    return delta;
}

KalshiOrderbookSnapshot KalshiMessageParser::fillKalshiOrderbookSnapshot(simdjson::dom::element& doc){
    KalshiOrderbookSnapshot snapshot;
    // simdjson::dom::element msg = doc["msg"];

    // simdjson::dom::array yes_levels = msg["yes_dollars_fp"].get_array();
    // simdjson::dom::array no_levels = msg["no_dollars_fp"].get_array();

    // snapshot.yes_levels = {};
    // for (simdjson::dom::array level : yes_levels){
    //     std::string_view price_str = level.at(0).get_string();
    //     std::string_view quantity_str = level.at(1).get_string();

    //     KalshiPriceLevel pl;
    //     pl.price = parsePrice(price_str);

    //     pl.quantity = 0;
    //     for (int i = 0; i < quantity_str.size(); i++){
    //         if (quantity_str[i] == '.'){
    //             continue;
    //         }
    //         pl.quantity *= 10;
    //         pl.quantity += static_cast<int>(quantity_str[i] - '0');
    //     }
    //     snapshot.yes_levels.push_back(pl);
    // }

    // snapshot.no_levels = {};
    // for (simdjson::dom::array level : no_levels){
    //     std::string_view price_str = level.at(0).get_string();
    //     std::string_view quantity_str = level.at(1).get_string();

    //     KalshiPriceLevel pl;
    //     pl.price = parsePrice(price_str);

    //     pl.quantity = 0;
    //     for (int i = 0; i < quantity_str.size(); i++){
    //         if (quantity_str[i] == '.'){
    //             continue;
    //         }
    //         pl.quantity *= 10;
    //         pl.quantity += static_cast<int>(quantity_str[i] - '0');
    //     }
    //     snapshot.no_levels.push_back(pl);
    // }
    fillKalshiOrderbookSnapshot(doc, snapshot);
    return snapshot;
}

KalshiOrderbookSnapshot KalshiMessageParser::fillKalshiOrderbookSnapshotFromRest(simdjson::dom::element& doc){
    KalshiOrderbookSnapshot snapshot;
    simdjson::dom::element msg = doc["orderbook_fp"];

    simdjson::dom::array yes_levels = msg["yes_dollars"].get_array();
    simdjson::dom::array no_levels = msg["no_dollars"].get_array();

    snapshot.yes_levels = {};
    for (simdjson::dom::array level : yes_levels){
        std::string_view price_str = level.at(0).get_string();
        std::string_view quantity_str = level.at(1).get_string();

        KalshiPriceLevel pl;
        pl.price = parsePrice(price_str);

        pl.quantity = 0;
        for (int i = 0; i < quantity_str.size(); i++){
            if (quantity_str[i] == '.'){
                continue;
            }
            pl.quantity *= 10;
            pl.quantity += static_cast<int>(quantity_str[i] - '0');
        }
        snapshot.yes_levels.push_back(pl);
    }

    snapshot.no_levels = {};
    for (simdjson::dom::array level : no_levels){
        std::string_view price_str = level.at(0).get_string();
        std::string_view quantity_str = level.at(1).get_string();

        KalshiPriceLevel pl;
        pl.price = parsePrice(price_str);

        pl.quantity = 0;
        for (int i = 0; i < quantity_str.size(); i++){
            if (quantity_str[i] == '.'){
                continue;
            }
            pl.quantity *= 10;
            pl.quantity += static_cast<int>(quantity_str[i] - '0');
        }
        snapshot.no_levels.push_back(pl);
    }
    return snapshot;
}


void KalshiMessageParser::fillKalshiOrderbookDelta(simdjson::dom::element& doc, KalshiOrderbookDelta& delta){
    simdjson::dom::element msg = doc["msg"];

    // fill price
    std::string_view price_str = msg["price_dollars"].get_string();
    delta.price = parsePrice(price_str); 

    // fill quantity
    std::string_view delta_str = msg["delta_fp"].get_string();
    int i = 0;
    bool neg_delta = false;
    delta.quantity_hundredths = 0;
    if (delta_str[0] == '-'){
        neg_delta = true;
        i = 1;
    }
    for (; i < delta_str.size(); i++){
        if (delta_str[i] == '.'){
            continue;
        }
        delta.quantity_hundredths *= 10;
        delta.quantity_hundredths += static_cast<uint16_t>(delta_str[i] - '0');
    }
    if (neg_delta){
        delta.quantity_hundredths *= -1;
    }

    // fill side
    std::string_view side_str = msg["side"].get_string();
    if (side_str[0] == 'n'){
        delta.side = KalshiSide::No;
    } else {
        delta.side = KalshiSide::Yes;
    }

    // fill ts_ms
    // might have to undo this .get_string()
    delta.ts_ms = msg["ts_ms"].get_uint64();
}


void KalshiMessageParser::fillKalshiOrderbookSnapshot(simdjson::dom::element& doc, KalshiOrderbookSnapshot& snapshot){
    simdjson::dom::element msg = doc["msg"];

    simdjson::dom::array yes_levels = msg["yes_dollars_fp"].get_array();
    simdjson::dom::array no_levels = msg["no_dollars_fp"].get_array();

    snapshot.yes_levels = {};
    for (simdjson::dom::array level : yes_levels){
        std::string_view price_str = level.at(0).get_string();
        std::string_view quantity_str = level.at(1).get_string();

        KalshiPriceLevel pl;
        pl.price = parsePrice(price_str);

        pl.quantity = 0;
        for (int i = 0; i < quantity_str.size(); i++){
            if (quantity_str[i] == '.'){
                continue;
            }
            pl.quantity *= 10;
            pl.quantity += static_cast<int>(quantity_str[i] - '0');
        }
        snapshot.yes_levels.push_back(pl);
    }

    snapshot.no_levels = {};
    for (simdjson::dom::array level : no_levels){
        std::string_view price_str = level.at(0).get_string();
        std::string_view quantity_str = level.at(1).get_string();

        KalshiPriceLevel pl;
        pl.price = parsePrice(price_str);

        pl.quantity = 0;
        for (int i = 0; i < quantity_str.size(); i++){
            if (quantity_str[i] == '.'){
                continue;
            }
            pl.quantity *= 10;
            pl.quantity += static_cast<int>(quantity_str[i] - '0');
        }
        snapshot.no_levels.push_back(pl);
    }
}