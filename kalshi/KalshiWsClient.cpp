#include "KalshiWsClient.hpp"
#include "KalshiAuth.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <cpr/cpr.h>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp=net::ip::tcp;


KalshiWsClient::KalshiWsClient(const KalshiAuth& auth, std::string& ws_host, std::string& connection_path): 
    auth_(auth), ws_host_(ws_host), connection_path_(connection_path) {}


using MessageCallback = std::function<void(const std::string& msg)>;
void KalshiWsClient::start(MessageCallback on_message, std::string& ticker){
    on_message_ = on_message;
    ticker_ = ticker;
    worker_thread_ = std::thread([this]{run();});
}

void KalshiWsClient::run(){

    try {
        net::io_context ioc;
        ssl::context ctx{ssl::context::tlsv12_client};
        ctx.set_default_verify_paths();

        beast::ssl_stream<beast::tcp_stream> stream{ioc, ctx};
        websocket::stream<beast::ssl_stream<beast::tcp_stream>&> ws{stream};

        connect(ioc, stream, ws);


        // subscribeToMarketFeed();

    } catch(const std::exception& e) {
        running_ = false;
        std::cerr << "Kalshi Websocket Error: " << e.what() << std::endl;
    }
}

void KalshiWsClient::connect(net::io_context& ioc, beast::ssl_stream<beast::tcp_stream>& stream, websocket::stream<beast::ssl_stream<beast::tcp_stream>&>& ws){
    std::string port = "443";

    tcp::resolver resolver{ioc};
    auto const results = resolver.resolve(ws_host_, port);

    beast::get_lowest_layer(stream).connect(results);

    // TLS handshake
    if (!SSL_set_tlsext_host_name(stream.native_handle(), ws_host_.c_str())){
        throw beast::system_error{
            beast::error_code{
                static_cast<int>(::ERR_get_error()),
                net::error::get_ssl_category()
            }
        };
    }
    
    stream.handshake(ssl::stream_base::client);

    cpr::Header headers = auth_.createHeader("GET", connection_path_);

    ws.set_option(websocket::stream_base::decorator(
        [&](websocket::request_type&req){
            for (const auto& [key, val] : headers) req.set(key, val);
        }
    ));

    ws.handshake(ws_host_, connection_path_);
}