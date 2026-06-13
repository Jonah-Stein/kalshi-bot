#include "kalshi/KalshiAuth.hpp"
#include "kalshi/KalshiRestClient.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <stdexcept>

std::string readFile(const std::string& path){
    // input file stream
    std::ifstream file(path);
    if (!file){
        throw std::runtime_error("Couldn't open file");
    }

    // output string stream
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

int main(){
    
    std::string base_url = "https://external-api.kalshi.com";
    const char* api_key_env = std::getenv("KALSHI_API_KEY");
    if (!api_key_env){
        throw std::runtime_error("api key not set\n");
    }
    std::string api_key = api_key_env;

    const char* pem_path = std::getenv("KALSHI_PEM_PATH");
    if (!pem_path){
        throw std::runtime_error("PEM file not found");
    }
    std::string pem = readFile(pem_path);

    KalshiAuth auth(pem, api_key);
    KalshiRestClient rest_client(auth, base_url);


    rest_client.printSeriesInfo("KXHIGHNY");
}