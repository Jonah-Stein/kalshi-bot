#include "Orderbook.hpp"
#include "../kalshi/KalshiWsMessages.hpp"
#include <cstdint>


Orderbook::Orderbook(uint16_t price_denominations){
    contracts_at_price_.resize(price_denominations);
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

void Orderbook::applySnapshot(KalshiOrderbookSnapshot& snapshot){}

void Orderbook::applyNoDelta(KalshiOrderbookDelta& delta){
    uint16_t price_hundredths_cents = 10000 - delta.price_hundredths_cents;
    contracts_at_price_[price_hundredths_cents] += delta.quantity_hundredths;
    if (contracts_at_price_[price_hundredths_cents] == 0){
        while (!asks_.empty() && contracts_at_price_[asks_.top()] == 0){
            asks_.pop();
        } 
    }
    else if(contracts_at_price_[price_hundredths_cents] == delta.quantity_hundredths){
        asks_.push(price_hundredths_cents);
    }
}

void Orderbook::applyYesDelta(KalshiOrderbookDelta& delta){
    contracts_at_price_[delta.price_hundredths_cents] += delta.quantity_hundredths;
    if (contracts_at_price_[delta.price_hundredths_cents] == 0){
        while (!bids_.empty() && contracts_at_price_[bids_.top()] == 0){
            bids_.pop();
        }
    }
    else if (contracts_at_price_[delta.price_hundredths_cents] == delta.quantity_hundredths){
        bids_.push(delta.price_hundredths_cents);
    }
}