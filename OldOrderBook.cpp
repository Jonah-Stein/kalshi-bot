#include <vector>


class OrderBook {
public:
    explicit OrderBook(int step_size){
        int num_cells = 1/step_size;
        contracts.resize(1/step_size);
        bid_idx = 0;
        ask_idx = num_cells - 1;
    }

private:
    std::vector<uint32_t> contracts;
    int bid_idx;
    int ask_idx;

}