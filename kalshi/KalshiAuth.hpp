#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include <openssl/evp.h>
#include <cpr/cpr.h>
#include <simdjson.h>

class KalshiAuth {
public:
    // need to remove the baseurl from here
    KalshiAuth(const std::string& pem, const std::string& api_key);
    ~KalshiAuth();

    cpr::Header createHeader(const std::string& http_method, const std::string& req_path) const;
private: 
    EVP_PKEY* private_key_; // used for signing
    std::string api_access_token_;

    EVP_PKEY* loadPrivateKeyFromPem(const std::string& pem); 
    std::vector<unsigned char> signRsaPssSha256(const std::string& message) const;

    std::string base64Encode(const unsigned char* data, size_t len) const;

    int64_t timestampMs() const;

};