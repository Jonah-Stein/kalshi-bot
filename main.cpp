#include "kalshi/KalshiAuth.hpp"
#include "kalshi/KalshiRestClient.hpp"
#include "kalshi/KalshiWsClient.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <stdexcept>
#include <queue>
#include <format>


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

std::string readEnvVar(const std::string& var_name){
    const char* raw_var = std::getenv(var_name.c_str());
    if (!raw_var){
        throw std::runtime_error(std::format("{} not set", var_name));
    }
    return raw_var;
}


int main(){
    std::string base_url = readEnvVar("KALSHI_API_BASE_URL");
    std::string api_key = readEnvVar("KALSHI_API_KEY");
    std::string pem_path = readEnvVar("KALSHI_PEM_PATH");
    std::string ws_url = readEnvVar("KALSHI_WS_URL");
    std::string ws_connection_path = readEnvVar("KALSHI_WS_CONNECTION_PATH");

    // Have to load pem file
    std::string pem = readFile(pem_path);

    KalshiAuth auth(pem, api_key);
    KalshiRestClient rest_client(auth, base_url);
    rest_client.printSeriesInfo("KXHIGHNY");
    

    // KalshiWsClient ws_client(auth, ws_url, ws_connection_path);

    // std::queue<std::string> q;
    // auto readIntoQueue = [&q](const std::string& msg){

    // }

    

}