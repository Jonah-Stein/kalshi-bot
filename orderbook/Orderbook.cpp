#include "Orderbook.hpp"
#include "../kalshi/KalshiWsMessages.hpp"
#include <cstdint>
#include <iostream>


Orderbook::Orderbook(uint16_t price_denominations){
    price_denominations_ = price_denominations;
    contracts_at_price_.resize(price_denominations, 0);
    bid_price_level_ = 0;
    ask_price_level_ = price_denominations_;
}

// when finding the net top for either bid or ask,
// need to verify that qunatity isn't 0.
// continue popping until quantity > 0
void Orderbook::applyDelta(KalshiOrderbookDelta& delta){
    if (delta.side == KalshiSide::No){
        applyNoDelta(delta);
    } else {
        applyYesDelta(delta);
    }
}

void Orderbook::applySnapshot(KalshiOrderbookSnapshot& snapshot){
    // need to reset orderbook
    std::fill(contracts_at_price_.begin(), contracts_at_price_.end(), 0);
    bid_price_level_ = 0;
    ask_price_level_ = price_denominations_;

    // can set the bid and ask price levels like so since the api 
    // returns the levels in sorted order.
    for(KalshiPriceLevel& level: snapshot.yes_levels){
        contracts_at_price_[level.price] = level.quantity;
        bid_price_level_ = level.price;
    }
    for (KalshiPriceLevel& level: snapshot.no_levels){
        uint16_t ask_price = price_denominations_ - level.price;
        contracts_at_price_[ask_price] = level.quantity;
        ask_price_level_ = ask_price;
    }
}

void Orderbook::applyNoDelta(KalshiOrderbookDelta& delta){
    uint16_t price_hundredths_cents = price_denominations_ - delta.price;
    contracts_at_price_[price_hundredths_cents] += delta.quantity_hundredths;
    if (contracts_at_price_[price_hundredths_cents] == 0){
        while (ask_price_level_ < price_denominations_ - 1 && contracts_at_price_[ask_price_level_] == 0){
            ask_price_level_++;
        } 
    }
    else if(price_hundredths_cents < ask_price_level_){
        ask_price_level_ = price_hundredths_cents;
    }
}

void Orderbook::applyYesDelta(KalshiOrderbookDelta& delta){
    contracts_at_price_[delta.price] += delta.quantity_hundredths;
    if (contracts_at_price_[delta.price] == 0){
        while (bid_price_level_ > 1 && contracts_at_price_[bid_price_level_] == 0){
            bid_price_level_--;
        }
    }
    else if (delta.price > bid_price_level_){
        bid_price_level_ = delta.price;
    }
}

// future feature: format like kalshi snapshot
void Orderbook::printSnapshot(){
    for (int i = 0; i < contracts_at_price_.size(); i++){
        std::cout<< i << " : " << contracts_at_price_[i] << "\n";
    }
    std::cout << "Bid price: " << bid_price_level_ << "\n";
    std::cout << "Ask price: " << ask_price_level_ << "\n";
}