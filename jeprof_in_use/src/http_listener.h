#pragma once

#include <memory>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/make_unique.hpp>
#include <boost/optional.hpp>
#include "utils.h"

namespace beast = boost::beast;                 // from <boost/beast.hpp>
namespace http = beast::http;                   // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;         // from <boost/beast/websocket.hpp>
namespace net = boost::asio;                    // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>


/**
 * Accepts incoming connections and launches the sessions
 */
class HttpListener : public std::enable_shared_from_this<HttpListener> {
public:
    HttpListener(net::io_context &ioc,tcp::endpoint endpoint);
    /**
     * Start accepting incoming connections
     */
    void run();

private:
    /**
     * 接收请求
     */
    void doAccept();

    /**
     * 请求处理
     * @param ec
     * @param socket
     */
    void process(beast::error_code ec, tcp::socket socket);

private:
    net::io_context &ioc_;
    tcp::acceptor acceptor_;
};