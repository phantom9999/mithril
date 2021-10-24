#pragma once

#include <memory>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/error.hpp>

/**
 * Accepts incoming connections and launches the sessions
 */
class HttpListener : public std::enable_shared_from_this<HttpListener> {
public:
    HttpListener(boost::asio::io_context &ioc, boost::asio::ip::tcp::endpoint endpoint);
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
    void process(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);

private:
    boost::asio::io_context &ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;
};