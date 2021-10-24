#include "http_session.h"

#include <memory>
#include <vector>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include "utils.h"
#include <jemalloc/jemalloc.h>
#include <glog/logging.h>

HttpSession::HttpSession(boost::asio::ip::tcp::socket &&socket) : stream_(std::move(socket)), queue_(*this) {
    this->handlers_.insert({"/", [this]() { return processNormal(); }});
    this->handlers_.insert({"/dump", [this]() { return processDump(); }});
    this->handlers_.insert({"/leak", [this]() { return processLeak(); }});
}

void HttpSession::run() {
    // We need to be executing within a strand to perform async operations
    // on the I/O objects in this session. Although not strictly necessary
    // for single-threaded contexts, this example code is written to be
    // thread-safe by default.
    boost::asio::dispatch(
            stream_.get_executor(),
            boost::beast::bind_front_handler(
                    &HttpSession::doRead,
                    this->shared_from_this()));
}

void HttpSession::doRead() {
    // Construct a new parser for each message
    parser_.emplace();

    // Apply a reasonable limit to the allowed size
    // of the body in bytes to prevent abuse.
    parser_->body_limit(10000);

    // Set the timeout.
    stream_.expires_after(std::chrono::seconds(30));

    // Read a request using the parser-oriented interface
    boost::beast::http::async_read(
            stream_,
            buffer_,
            *parser_,
            boost::beast::bind_front_handler(
                    &HttpSession::onRead,
                    shared_from_this()));
}


void HttpSession::onRead(boost::beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == boost::beast::http::error::end_of_stream)
        return doClose();

    if (ec)
        return fail(ec, "read");

    // Send the response
    handleRequest(parser_->release(), queue_);

    // If we aren't at the queue limit, try to pipeline another request
    if (!queue_.isFull())
        doRead();
}

void HttpSession::onWrite(bool close, boost::beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec)
        return fail(ec, "write");

    if (close) {
        // This means we should close the connection, usually because
        // the response indicated the "Connection: close" semantic.
        return doClose();
    }

    // Inform the queue that a write completed
    if (queue_.onWrite()) {
        // Read another request
        doRead();
    }
}

void HttpSession::doClose() {
    // Send a TCP shutdown
    boost::beast::error_code ec;
    stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

    // At this point the connection is closed gracefully
}

void HttpSession::handleRequest(boost::beast::http::request<boost::beast::http::string_body> &&req, HttpSession::PipelineQueue& send) {
    // AAA<decltype(send)> aa;
    // Returns a bad request response
    auto const bad_request =
            [&req](boost::beast::string_view why) {
                boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::bad_request, req.version()};
                res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(boost::beast::http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = std::string(why);
                res.prepare_payload();
                return res;
            };

    // Returns a not found response
    auto const not_found =
            [&req](boost::beast::string_view target) {
                boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::not_found, req.version()};
                res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(boost::beast::http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = "The resource '" + std::string(target) + "' was not found.";
                res.prepare_payload();
                return res;
            };
    auto const status_ok =
            [&req](boost::beast::string_view what) {
                boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::ok, req.version()};
                res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(boost::beast::http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = std::string(what);
                res.prepare_payload();
                return res;
            };

    // Make sure we can handle the method
    if (req.method() != boost::beast::http::verb::get && req.method() != boost::beast::http::verb::head) {
        return send(bad_request("Unknown HTTP-method"));
    }

    // Request path must be absolute and not contain "..".
    if (req.target().empty() || req.target()[0] != '/' || req.target().find("..") != boost::beast::string_view::npos) {
        return send(bad_request("Illegal request-target"));
    }
    auto handler_it = handlers_.find(std::string(req.target()));
    if (handler_it == handlers_.end()) {
        return send(not_found(req.target()));
    }
    std::string result = handler_it->second();
    return send(status_ok(result));
}

std::string HttpSession::processNormal() {
    LOG(INFO) << "visit";
    return "use http://www.webgraphviz.com/ ";
}

std::string HttpSession::processLeak() {
    int32_t* leak = new int32_t[1024*256];
    LOG(INFO) << "leak address " << leak;
    return "memory leak";
}

std::string HttpSession::processDump() {
    if (mallctl("prof.dump", nullptr, nullptr, nullptr, 0) == 0) {
        LOG(INFO) << "dump sucess";
        return "dump success";
    } else {
        LOG(INFO) << "dump sucess";
        return "dump fail";
    }
}


