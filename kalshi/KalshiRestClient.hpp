#include "KalshiAuth.hpp"

#include <string>

#include <cpr/cpr.h>
#include <simdjson.h>

class KalshiRestClient {
public:
    KalshiRestClient(const KalshiAuth& auth, std::string& base_url);

    void printSeriesInfo(const std::string& series_ticker);

private:
    const KalshiAuth& auth_;
    std::string base_url_;
    simdjson::dom::parser json_parser_;

    cpr::Response getRequest(const std::string& path);
    simdjson::dom::element parseResponse(const cpr::Response& resp);
}