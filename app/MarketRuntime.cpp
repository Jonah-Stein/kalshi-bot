#include "MarketRuntime.hpp"

#include <string_view>

MarketRuntime::MarketRuntime(
    DoubleBuffer<OrderbookMetrics>& metrics,
    KalshiAuth& auth,
    std::string& ws_host,
    std::string& connection_path,
    uint16_t price_denominations
)
    : ring_buffer_(1024)
    , parser_(2) // could un-hardcode this
    , kalshi_ws_client_(auth, ws_host, connection_path)
    , orderbook_(price_denominations)
    , metrics_buffer_(metrics)
{}

void MarketRuntime::start(std::string& ticker){
    running_.store(true, std::memory_order_release);
    kalshi_ws_client_.start([this](std::string& msg){handle_message(msg);}, ticker);
    orderbook_thread_ = std::thread([this]{consume_loop();});
}


void MarketRuntime::stop(){
    kalshi_ws_client_.stop();
    orderbook_thread_.join();
}


// private
void MarketRuntime::handle_message(std::string& msg){
    simdjson::dom::element doc = parser_.parseResponse(msg);
    std::string_view type = doc["type"].get_string();

    OrderbookUpdate* write_slot = ring_buffer_.GetWriteSlot();

    switch (type.size()){
        case 15: {
            // delta
            write_slot->type = KalshiMessageType::Delta;
            parser_.fillKalshiOrderbookDelta(doc, write_slot->delta);
            ring_buffer_.FinishWrite();
            break;
        }
        case 18: {
            //snapshot
            write_slot->type = KalshiMessageType::Snapshot;
            parser_.fillKalshiOrderbookSnapshot(doc, write_slot->snapshot);
            ring_buffer_.FinishWrite();
            break;
        }
        case 4: {
            //stop
            write_slot->type = KalshiMessageType::Stop;
            running_.store(false, std::memory_order_release);
            break;
        }
    }
    messages_received_++;
}


void MarketRuntime::report_metrics(){
    OrderbookMetrics& slot = metrics_buffer_.write_slot();
    slot.messages_received = messages_received_;
    slot.messages_processed = messages_processed_;
    slot.bid_price = orderbook_.best_bid();
    slot.bid_quantity = orderbook_.size_at(slot.bid_price);
    slot.ask_price = orderbook_.best_ask();
    slot.ask_quantity = orderbook_.size_at(slot.ask_price);
    metrics_buffer_.publish();
}

void MarketRuntime::consume_loop(){
    while (ring_buffer_.TryRead() != nullptr || !running_.load(std::memory_order_acquire)){
        if (ring_buffer_.TryRead() == nullptr){
            continue;
        }
        OrderbookUpdate* slot = ring_buffer_.TryRead();

        switch(slot->type){
            case KalshiMessageType::Delta :{
                orderbook_.applyDelta(slot->delta);
                break;
            }
            case KalshiMessageType::Snapshot :{
                orderbook_.applySnapshot(slot->snapshot);
                break;
            }
            case KalshiMessageType::Stop :{
                break;
            }
        }
        ring_buffer_.FinishRead();
        
        // TODO: Add reporting metrics into the doublebuffer
        report_metrics();
    }
}