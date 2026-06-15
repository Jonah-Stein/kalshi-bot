#pragma once

#include "KalshiAuth.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/io_context.hpp>


namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;

#include <string>
#include <thread>
#include <functional>
#include <atomic>

using MessageCallback = std::function<void(const std::string& msg)>;
class KalshiWsClient {
public:
    KalshiWsClient(const KalshiAuth& auth, std::string& ws_host, std::string& connection_path);

    
    void start(MessageCallback on_message, std::string& ticker);
    void stop();

private:
    const KalshiAuth& auth_;
    std::string ws_host_;
    std::string connection_path_;
    std::atomic<bool> running_{false};
    std::thread worker_thread_;
    MessageCallback on_message_;
    std::string ticker_;

    void run();
    void connect(net::io_context& ioc, beast::ssl_stream<beast::tcp_stream>& stream, websocket::stream<beast::ssl_stream<beast::tcp_stream>&>& ws);
    void subscribeToOrderBook(websocket::stream<beast::ssl_stream<beast::tcp_stream>&>&ws);
    void readLoop(websocket::stream<beast::ssl_stream<beast::tcp_stream>&>& ws);

};