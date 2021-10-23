#pragma once

#include <memory>
#include <vector>
#include <iostream>
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




// Handles an HTTP server connection
class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    // This queue is used for HTTP pipelining.
    class queue {
        enum {
            // Maximum number of responses we will queue
            limit = 8
        };

        // The type-erased, saved work item
        struct work {
            virtual ~work() = default;

            virtual void operator()() = 0;
        };

        HttpSession &self_;
        std::vector<std::unique_ptr<work>> items_;

    public:
        explicit queue(HttpSession &self) : self_(self) {
            static_assert(limit > 0, "queue limit must be positive");
            items_.reserve(limit);
        }

        // Returns `true` if we have reached the queue limit
        bool is_full() const {
            return items_.size() >= limit;
        }

        // Called when a message finishes sending
        // Returns `true` if the caller should initiate a read
        bool on_write() {
            BOOST_ASSERT(!items_.empty());
            auto const was_full = is_full();
            items_.erase(items_.begin());
            if (!items_.empty())
                (*items_.front())();
            return was_full;
        }

        // Called by the HTTP handler to send a response.
        template<bool isRequest, class Body, class Fields>
        void operator()(http::message<isRequest, Body, Fields> &&msg) {
            // This holds a work item
            struct work_impl : work {
                HttpSession &self_;
                http::message<isRequest, Body, Fields> msg_;

                work_impl(
                        HttpSession &self,
                        http::message<isRequest, Body, Fields> &&msg)
                        : self_(self), msg_(std::move(msg)) {
                }

                void
                operator()() {
                    http::async_write(self_.stream_, msg_,
                            beast::bind_front_handler(
                                    &HttpSession::on_write,
                                    self_.shared_from_this(),
                                    msg_.need_eof()));
                }
            };

            // Allocate and store the work
            items_.push_back(
                    boost::make_unique<work_impl>(self_, std::move(msg)));

            // If there was no previous work, start this one
            if (items_.size() == 1)
                (*items_.front())();
        }
    };

private:

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    queue queue_;

    // The parser is stored in an optional container so we can
    // construct it from scratch it at the beginning of each new message.
    boost::optional<http::request_parser<http::string_body>> parser_;

public:
    // Take ownership of the socket
    HttpSession(tcp::socket &&socket);

    // Start the session
    void run();

private:
    void do_read();

    void on_read(beast::error_code ec, std::size_t bytes_transferred);

    void on_write(bool close, beast::error_code ec, std::size_t bytes_transferred);

    void do_close();
};

template<typename TT>
struct AAA;


// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
// template<class Send>
void handle_request(http::request<http::string_body> &&req, HttpSession::queue& send) {
    // AAA<decltype(send)> aa;
    // Returns a bad request response
    auto const bad_request =
            [&req](beast::string_view why) {
                http::response<http::string_body> res{http::status::bad_request, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = std::string(why);
                res.prepare_payload();
                return res;
            };

    // Returns a not found response
    auto const not_found =
            [&req](beast::string_view target) {
                http::response<http::string_body> res{http::status::not_found, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = "The resource '" + std::string(target) + "' was not found.";
                res.prepare_payload();
                return res;
            };

    // Returns a server error response
    auto const server_error =
            [&req](beast::string_view what) {
                http::response<http::string_body> res{http::status::internal_server_error, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = "An error occurred: '" + std::string(what) + "'";
                res.prepare_payload();
                return res;
            };
    auto const status_ok =
            [&req](beast::string_view what) {
                http::response<http::string_body> res{http::status::ok, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = std::string(what);
                res.prepare_payload();
                return res;
            };

    // Make sure we can handle the method
    if (req.method() != http::verb::get && req.method() != http::verb::head) {
        return send(bad_request("Unknown HTTP-method"));
    }

    // Request path must be absolute and not contain "..".
    if (req.target().empty() || req.target()[0] != '/' || req.target().find("..") != beast::string_view::npos) {
        return send(bad_request("Illegal request-target"));
    }
    std::cout << req.target() << std::endl;
    send(status_ok("hello world"));
}


