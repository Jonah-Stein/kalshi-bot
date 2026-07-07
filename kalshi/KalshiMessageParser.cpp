#include "KalshiMessageParser.hpp"
#include "KalshiWsMessages.hpp"

#include <format>
#include <stdexcept>
#include <simdjson.h>


// simdjson::dom::element KalshiMessageParser::parseResponse(std::string& res){
//     simdjson::dom::element doc;
//     auto err = json_parser_.parse(res).get(doc);
//     if (err) {
//         throw std::runtime_error(
//             std::format("JSON parse error: {}", simdjson::error_message(err))
//         );
//     }
//     return doc;
// }

simdjson::ondemand::document KalshiMessageParser::parseResponse(simdjson::padded_string& padded){
    simdjson::ondemand::document doc = json_parser_.iterate(padded);

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


// KalshiOrderbookDelta KalshiMessageParser::fillKalshiOrderbookDelta(simdjson::dom::element& doc){
//     simdjson::dom::element msg = doc["msg"];
//     KalshiOrderbookDelta delta;

//     // fill price
//     std::string_view price_str = msg["price_dollars"].get_string();
//     delta.price = parsePrice(price_str); 

//     // fill quantity
//     std::string_view delta_str = msg["delta_fp"].get_string();
//     int i = 0;
//     bool neg_delta = false;
//     delta.quantity_hundredths = 0;
//     if (delta_str[0] == '-'){
//         neg_delta = true;
//         i = 1;
//     }
//     for (; i < delta_str.size(); i++){
//         if (delta_str[i] == '.'){
//             continue;
//         }
//         delta.quantity_hundredths *= 10;
//         delta.quantity_hundredths += static_cast<uint16_t>(delta_str[i] - '0');
//     }
//     if (neg_delta){
//         delta.quantity_hundredths *= -1;
//     }

//     // fill side
//     std::string_view side_str = msg["side"].get_string();
//     if (side_str[0] == 'n'){
//         delta.side = KalshiSide::No;
//     } else {
//         delta.side = KalshiSide::Yes;
//     }

//     // fill ts_ms
//     // might have to undo this .get_string()
//     delta.ts_ms = msg["ts_ms"].get_uint64();

//     return delta;
// }

// KalshiOrderbookSnapshot KalshiMessageParser::fillKalshiOrderbookSnapshot(simdjson::dom::element& doc){
//     KalshiOrderbookSnapshot snapshot;
//     simdjson::dom::element msg = doc["msg"];

//     simdjson::dom::array yes_levels = msg["yes_dollars_fp"].get_array();
//     simdjson::dom::array no_levels = msg["no_dollars_fp"].get_array();

//     snapshot.yes_levels = {};
//     for (simdjson::dom::array level : yes_levels){
//         std::string_view price_str = level.at(0).get_string();
//         std::string_view quantity_str = level.at(1).get_string();

//         KalshiPriceLevel pl;
//         pl.price = parsePrice(price_str);

//         pl.quantity = 0;
//         for (int i = 0; i < quantity_str.size(); i++){
//             if (quantity_str[i] == '.'){
//                 continue;
//             }
//             pl.quantity *= 10;
//             pl.quantity += static_cast<int>(quantity_str[i] - '0');
//         }
//         snapshot.yes_levels.push_back(pl);
//     }

//     snapshot.no_levels = {};
//     for (simdjson::dom::array level : no_levels){
//         std::string_view price_str = level.at(0).get_string();
//         std::string_view quantity_str = level.at(1).get_string();

//         KalshiPriceLevel pl;
//         pl.price = parsePrice(price_str);

//         pl.quantity = 0;
//         for (int i = 0; i < quantity_str.size(); i++){
//             if (quantity_str[i] == '.'){
//                 continue;
//             }
//             pl.quantity *= 10;
//             pl.quantity += static_cast<int>(quantity_str[i] - '0');
//         }
//         snapshot.no_levels.push_back(pl);
//     }
//     return snapshot;
// }

// KalshiOrderbookSnapshot KalshiMessageParser::fillKalshiOrderbookSnapshotFromRest(simdjson::dom::element& doc){
//     KalshiOrderbookSnapshot snapshot;
//     simdjson::dom::element msg = doc["orderbook_fp"];

//     simdjson::dom::array yes_levels = msg["yes_dollars"].get_array();
//     simdjson::dom::array no_levels = msg["no_dollars"].get_array();

//     snapshot.yes_levels = {};
//     for (simdjson::dom::array level : yes_levels){
//         std::string_view price_str = level.at(0).get_string();
//         std::string_view quantity_str = level.at(1).get_string();

//         KalshiPriceLevel pl;
//         pl.price = parsePrice(price_str);

//         pl.quantity = 0;
//         for (int i = 0; i < quantity_str.size(); i++){
//             if (quantity_str[i] == '.'){
//                 continue;
//             }
//             pl.quantity *= 10;
//             pl.quantity += static_cast<int>(quantity_str[i] - '0');
//         }
//         snapshot.yes_levels.push_back(pl);
//     }

//     snapshot.no_levels = {};
//     for (simdjson::dom::array level : no_levels){
//         std::string_view price_str = level.at(0).get_string();
//         std::string_view quantity_str = level.at(1).get_string();

//         KalshiPriceLevel pl;
//         pl.price = parsePrice(price_str);

//         pl.quantity = 0;
//         for (int i = 0; i < quantity_str.size(); i++){
//             if (quantity_str[i] == '.'){
//                 continue;
//             }
//             pl.quantity *= 10;
//             pl.quantity += static_cast<int>(quantity_str[i] - '0');
//         }
//         snapshot.no_levels.push_back(pl);
//     }
//     return snapshot;
// }


KalshiOrderbookDelta KalshiMessageParser::fillKalshiOrderbookDelta(simdjson::ondemand::object& msg){
    KalshiOrderbookDelta delta;

    std::string_view discard;
    // discard market_ticker, market_id
    msg["market_ticker"].get_string().get(discard);
    msg["market_id"].get_string().get(discard);

    // fill price
    std::string_view price_str;
    msg["price_dollars"].get_string().get(price_str);
    delta.price = parsePrice(price_str); 

    // fill quantity
    std::string_view delta_str;
    msg["delta_fp"].get_string().get(delta_str);
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
    std::string_view side_str;
    msg["side"].get_string().get(side_str);
    if (side_str[0] == 'n'){
        delta.side = KalshiSide::No;
    } else {
        delta.side = KalshiSide::Yes;
    }

    msg["ts"].get_string().get(discard);
    // fill ts_ms
    msg["ts_ms"].get_uint64().get(delta.ts_ms);

    return delta;
}

KalshiOrderbookSnapshot KalshiMessageParser::fillKalshiOrderbookSnapshot(simdjson::ondemand::object& msg){
    KalshiOrderbookSnapshot snapshot;

    std::string_view discard;
    msg["market_ticker"].get_string().get(discard);
    msg["market_id"].get_string().get(discard);

    simdjson::ondemand::array yes_levels;
    msg["yes_dollars_fp"].get_array().get(yes_levels);

    snapshot.yes_levels = {};
    snapshot.yes_levels.reserve(100);
    for (auto level_value : yes_levels){
        simdjson::ondemand::array level;
        level_value.get_array().get(level);

        std::string_view price_str;
        std::string_view quantity_str;
        uint8_t idx = 0;
        for (auto elem : level) {
            if (idx == 0){
                elem.get_string().get(price_str);
            } else {
                elem.get_string().get(quantity_str);
            }
            idx++;
        }

        // level.at(0).get_string().get(price_str);
        // level.at(1).get_string().get(quantity_str);

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

    simdjson::ondemand::array no_levels;
    msg["no_dollars_fp"].get_array().get(no_levels);

    snapshot.no_levels = {};
    snapshot.no_levels.reserve(100);
    for (auto level_value : no_levels){
        simdjson::ondemand::array level;
        level_value.get_array().get(level); 

        // std::string_view price_str;
        // level.at(0).get_string().get(price_str);
        // std::string_view quantity_str;
        // level.at(1).get_string().get(quantity_str);

        std::string_view price_str;
        std::string_view quantity_str;
        uint8_t idx = 0;
        for (auto elem : level) {
            if (idx == 0){
                elem.get_string().get(price_str);
            } else {
                elem.get_string().get(quantity_str);
            }
            idx++;
        }

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

KalshiOrderbookSnapshot KalshiMessageParser::fillKalshiOrderbookSnapshotFromRest(simdjson::ondemand::object& msg){
    KalshiOrderbookSnapshot snapshot;

    simdjson::ondemand::array yes_levels;
    msg["yes_dollars"].get_array().get(yes_levels);

    snapshot.yes_levels = {};
    snapshot.yes_levels.reserve(100);

    for (auto level_value : yes_levels){
        simdjson::ondemand::array level;
        level_value.get_array().get(level);

        // std::string_view price_str;
        // level.at(0).get_string().get(price_str);

        // std::string_view quantity_str;
        // level.at(1).get_string().get(quantity_str);

        std::string_view price_str;
        std::string_view quantity_str;
        uint8_t idx = 0;
        for (auto elem : level) {
            if (idx == 0){
                elem.get_string().get(price_str);
            } else {
                elem.get_string().get(quantity_str);
            }
            idx++;
        }

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

    simdjson::ondemand::array no_levels;
    msg["no_dollars"].get_array().get(no_levels);

    snapshot.no_levels = {};
    snapshot.no_levels.reserve(100);

    for (auto level_value : no_levels){
        simdjson::ondemand::array level;
        level_value.get_array().get(level);

        // std::string_view price_str;
        // level.at(0).get_string().get(price_str);
        // std::string_view quantity_str;
        // level.at(1).get_string().get(quantity_str);

        std::string_view price_str;
        std::string_view quantity_str;
        uint8_t idx = 0;
        for (auto elem : level) {
            if (idx == 0){
                elem.get_string().get(price_str);
            } else {
                elem.get_string().get(quantity_str);
            }
            idx++;
        }

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