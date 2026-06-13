#include "KalshiRestClient.hpp"
#include "KalshiAuth.hpp"

#include <iostream>
#include <string>
#include <format>
#include <stdexcept>

#include <cpr/cpr.h>
#include <simdjson.h>


KalshiRestClient::KalshiRestClient(const KalshiAuth& auth, std::string& base_url): auth_(auth), base_url_(base_url){}

void KalshiRestClient::printSeriesInfo(const std::string& series_ticker){
    std::string req_path = std::format("/trade-api/v2/series/{}", series_ticker);
    cpr::Response res = getRequest(req_path);
    std::cout<<res.text << "\n";
}

// create a get request -- going to return a generic json
cpr::Response KalshiRestClient::getRequest(const std::string& path){
    cpr::Header auth_header = auth_.createHeader("GET", path);

    return cpr::Get(
        cpr::Url{std::format("{}{}", base_url_, path)},
        auth_header
    );
}

// DOM is a document object model - in this case, an in-memory representation of the json
// document once its been parsed
simdjson::dom::element KalshiRestClient::parseResponse(const cpr::Response& resp){
    simdjson::dom::element doc;
    auto err = json_parser_.parse(resp.text).get(doc);
    if (err) {
        throw std::runtime_error(
            std::format("JSON parse error: {}", simdjson::error_message(err))
        );
    }
    return doc;
}