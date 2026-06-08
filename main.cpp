#include "KalshiClient.hpp"


int main(){
    
    std::string base_url = "https://external-api.kalshi.com";
    std::string api_key = "";
    std::string pem = "";
    KalshiClient client(pem, api_key, base_url);
    client.printSeriesInfo("KXHIGHNY");
}