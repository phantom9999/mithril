#include <boost/asio/strand.hpp>
#include "http_listener.h"
#include "http_session.h"
#include "utils.h"

HttpListener::HttpListener(boost::asio::io_context &ioc, boost::asio::ip::tcp::endpoint endpoint)
: ioc_(ioc), acceptor_(boost::asio::make_strand(ioc)) {
    boost::beast::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        fail(ec, "open");
        return;
    }

    // Allow address reuse
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
        fail(ec, "set_option");
        return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if (ec) {
        fail(ec, "bind");
        return;
    }

    // Start listening for connections
    acceptor_.listen(
            boost::asio::socket_base::max_listen_connections, ec);
    if (ec) {
        fail(ec, "listen");
        return;
    }
}

void HttpListener::run() {
    // We need to be executing within a strand to perform async operations
    // on the I/O objects in this session. Although not strictly necessary
    // for single-threaded contexts, this example code is written to be
    // thread-safe by default.
    boost::asio::dispatch(
            acceptor_.get_executor(),
            boost::beast::bind_front_handler(
                    &HttpListener::doAccept,
                    this->shared_from_this()));
}

void HttpListener::doAccept() {
    // The new connection gets its own strand
    acceptor_.async_accept(
            boost::asio::make_strand(ioc_),
            boost::beast::bind_front_handler(
                    &HttpListener::process,
                    shared_from_this()));
}

void HttpListener::process(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
    if (ec) {
        fail(ec, "accept");
    } else {
        // Create the http session and run it
        std::make_shared<HttpSession>(
                std::move(socket))->run();
    }

    // Accept another connection
    doAccept();
}