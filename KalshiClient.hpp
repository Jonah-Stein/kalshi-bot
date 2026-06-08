#include <string>
#include <vector>
#include <cstdint>

#include <openssl/evp.h>
#include <cpr/cpr.h>
#include <simdjson.h>

class KalshiClient {
public:
    KalshiClient(const std::string& pem, const std::string& api_key, const std::string& base_api_url);
    ~KalshiClient();
    void printSeriesInfo(const std::string& series_ticker);

private: 
    EVP_PKEY* private_key_; // used for signing
    std::string api_access_token_;
    std::string base_api_url_;
    simdjson::dom::parser json_parser_;

    std::string createAccessSignature(int64_t timestamp, const std::string& http_method, const std::string& req_path);
    

    EVP_PKEY* loadPrivateKeyFromPem(const std::string& pem); 
    std::vector<unsigned char> signRsaPssSha256(const std::string& message);

    std::string base64Encode(const unsigned char* data, size_t len);

    int64_t timestampMs();
    cpr::Response getRequest(const std::string& path);
    simdjson::dom::element parseResponse(const cpr::Response& resp);

};