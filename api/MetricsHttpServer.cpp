#include "MetricsHttpServer.hpp"
#include "metrics_json.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <utility>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace {

void set_cors(http::response<http::string_body>& res) {
    res.set(http::field::access_control_allow_origin, "*");
}

http::response<http::string_body> make_json_response(
    http::status status,
    unsigned version,
    bool keep_alive,
    std::string body
) {
    http::response<http::string_body> res{status, version};
    res.set(http::field::server, "kalshi-bot-metrics");
    res.set(http::field::content_type, "application/json");
    set_cors(res);
    res.keep_alive(keep_alive);
    res.body() = std::move(body);
    res.prepare_payload();
    return res;
}

http::response<http::string_body> handle_request(
    const http::request<http::string_body>& req,
    DoubleBuffer<OrderbookMetrics>& metrics
) {
    if (req.method() == http::verb::options) {
        http::response<http::string_body> res{http::status::no_content, req.version()};
        set_cors(res);
        res.set(http::field::access_control_allow_methods, "GET, OPTIONS");
        res.set(http::field::access_control_allow_headers, "Content-Type");
        res.keep_alive(req.keep_alive());
        res.prepare_payload();
        return res;
    }

    if (req.method() != http::verb::get) {
        return make_json_response(
            http::status::method_not_allowed,
            req.version(),
            req.keep_alive(),
            R"({"error":"method not allowed"})"
        );
    }

    const auto target = std::string(req.target());
    if (target == "/health") {
        return make_json_response(
            http::status::ok,
            req.version(),
            req.keep_alive(),
            R"({"status":"ok"})"
        );
    }

    if (target == "/metrics") {
        return make_json_response(
            http::status::ok,
            req.version(),
            req.keep_alive(),
            to_json(metrics.read())
        );
    }

    return make_json_response(
        http::status::not_found,
        req.version(),
        req.keep_alive(),
        R"({"error":"not found"})"
    );
}

} // namespace

MetricsHttpServer::MetricsHttpServer(
    DoubleBuffer<OrderbookMetrics>& metrics,
    std::string bind_address,
    std::uint16_t port
)
    : metrics_(metrics)
    , bind_address_(std::move(bind_address))
    , port_(port) {}

MetricsHttpServer::~MetricsHttpServer() {
    stop();
}

void MetricsHttpServer::start() {
    if (running_.exchange(true)) {
        return;
    }
    worker_ = std::thread([this] { run(); });
}

void MetricsHttpServer::stop() {
    if (!running_.exchange(false)) {
        if (worker_.joinable()) {
            worker_.join();
        }
        return;
    }

    // Unblock accept() with a local connection attempt.
    try {
        net::io_context ioc;
        tcp::socket socket{ioc};
        socket.connect(tcp::endpoint(
            net::ip::make_address(
                bind_address_ == "0.0.0.0" ? "127.0.0.1" : bind_address_
            ),
            port_
        ));
    } catch (...) {
        // Server may already be down.
    }

    if (worker_.joinable()) {
        worker_.join();
    }
}

void MetricsHttpServer::run() {
    try {
        net::io_context ioc{1};
        tcp::acceptor acceptor{
            ioc,
            {net::ip::make_address(bind_address_), port_}
        };
        acceptor.set_option(net::socket_base::reuse_address(true));

        std::cout << "Metrics HTTP server listening on "
                  << bind_address_ << ':' << port_ << '\n';

        while (running_.load(std::memory_order_acquire)) {
            beast::error_code ec;
            tcp::socket socket{ioc};
            acceptor.accept(socket, ec);
            if (ec) {
                if (!running_.load(std::memory_order_acquire)) {
                    break;
                }
                continue;
            }

            if (!running_.load(std::memory_order_acquire)) {
                break;
            }

            beast::flat_buffer buffer;
            http::request<http::string_body> req;
            http::read(socket, buffer, req, ec);
            if (ec) {
                continue;
            }

            auto res = handle_request(req, metrics_);
            http::write(socket, res, ec);
            socket.shutdown(tcp::socket::shutdown_send, ec);
        }
    } catch (const std::exception& e) {
        std::cerr << "Metrics HTTP server error: " << e.what() << '\n';
        running_.store(false, std::memory_order_release);
    }
}
