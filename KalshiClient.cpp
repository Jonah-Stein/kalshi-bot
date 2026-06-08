#include "KalshiClient.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <format>

#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <stdexcept>
#include <cpr/cpr.h>
#include <chrono>
#include <simdjson.h>


KalshiClient::KalshiClient(const std::string& pem, const std::string& api_key, const std::string& base_api_url){
    api_access_token_ = api_key;
    base_api_url_ = base_api_url;
    private_key_ = loadPrivateKeyFromPem(pem);
}

KalshiClient::~KalshiClient() {
    EVP_PKEY_free(private_key_);
}

void KalshiClient::printSeriesInfo(const std::string& series_ticker){
    std::string req_path = std::format("/trade-api/v2/series/{}", series_ticker);
    cpr::Response res = getRequest(req_path);
    std::cout<<res.text << "\n";
}


std::string KalshiClient::createAccessSignature(int64_t timestamp, const std::string& http_method, const std::string& req_path){
    std::string message_string = std::format("{}{}{}", timestamp, http_method, req_path);
    //sign
    std::vector<unsigned char> signed_message = signRsaPssSha256(message_string);
    //encode
    return base64Encode(signed_message.data(), signed_message.size());
}

EVP_PKEY* KalshiClient::loadPrivateKeyFromPem(const std::string& pem) {
    BIO* bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    if (!bio) throw std::runtime_error("BIO_new_mem_buf failed");

    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);

    if (!pkey) throw std::runtime_error("Failed to parse private key PEM");
    return pkey;
}

std::vector<unsigned char> KalshiClient::signRsaPssSha256(const std::string& message){
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_MD_CTX_new failed");

    // Init signing: SHA-256 + private key
    if (EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, private_key_) != 1){
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestSignInit failed");
    }

    // Configure RSA-PSS
    EVP_PKEY_CTX* pctx = EVP_MD_CTX_get_pkey_ctx(ctx);
    if (!pctx){
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("No pkey ctx");
    }

    if (EVP_PKEY_CTX_set_rsa_padding(pctx, RSA_PKCS1_PSS_PADDING) != 1){
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to set PSS padding");
    }

    if (EVP_PKEY_CTX_set_rsa_pss_saltlen(pctx, RSA_PSS_SALTLEN_DIGEST) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to set PSS salt length");
    }

    const auto* msg_bytes = reinterpret_cast<const unsigned char*>(message.data());
    size_t sig_len=0;

    // look into getting rid of the double pass here
    // ask how big the signature will be
    if (EVP_DigestSign(ctx, nullptr, &sig_len, msg_bytes, message.size()) != 1){
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestSign (size) faield");
    }
    
    std::vector<unsigned char> signature(sig_len);
    // actually sign
    if (EVP_DigestSign(ctx, signature.data(), &sig_len, msg_bytes, message.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestSign failed");
    }

    EVP_MD_CTX_free(ctx);
    signature.resize(sig_len);
    return signature;
}

std::string KalshiClient::base64Encode(const unsigned char* data, size_t len) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, mem);

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, data, static_cast<int>(len));
    BIO_flush(b64);

    BUF_MEM* buf = nullptr;
    BIO_get_mem_ptr(b64, &buf);

    std::string out(buf->data, buf->length);
    BIO_free_all(b64);
    return out;
}

int64_t KalshiClient::timestampMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

// create a get request -- going to return a generic json
cpr::Response KalshiClient::getRequest(const std::string& path){
    int64_t ts = timestampMs();
    std::string access_signature = createAccessSignature(ts, "GET", path);

    return cpr::Get(
        cpr::Url{std::format("{}{}", base_api_url_, path)},
        cpr::Header{
            {"KALSHI-ACCESS-KEY", api_access_token_},
            {"KALSHI-ACCESS-TIMESTAMP", std::to_string(ts)},
            {"KALSHI-ACCESS-SIGNATURE", access_signature}
        }
    );
}

// DOM is a document object model - in this case, an in-memory representation of the json
// document once its been parsed
simdjson::dom::element KalshiClient::parseResponse(const cpr::Response& resp){
    simdjson::dom::element doc;
    auto err = json_parser_.parse(resp.text).get(doc);
    if (err) {
        throw std::runtime_error(
            std::format("JSON parse error: {}", simdjson::error_message(err))
        );
    }
    return doc;
}
