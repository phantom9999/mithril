#pragma once

#include <memory>
#include <vector>
#include <boost/beast/http.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/optional.hpp>

/**
 * Handles an HTTP server connection
 */
class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    /**
     * This queue is used for HTTP pipelining.
     */
    class PipelineQueue {
        enum {
            // Maximum number of responses we will queue
            limit = 8
        };

        // The type-erased, saved work item
        struct work;

        HttpSession &self_;
        std::vector<std::shared_ptr<work>> items_;

    public:
        explicit PipelineQueue(HttpSession &self);

        /**
         * Returns `true` if we have reached the queue limit
         * @return
         */
        bool is_full() const;


        /**
         * Called when a message finishes sending
         * Returns `true` if the caller should initiate a read
         * @return
         */
        bool on_write();

        /**
         * Called by the HTTP handler to send a response.
         * @param msg
         */
        void operator()(boost::beast::http::response<boost::beast::http::string_body> &&msg);
    };

private:

    boost::beast::tcp_stream stream_;
    boost::beast::flat_buffer buffer_;
    PipelineQueue queue_;

    // The parser is stored in an optional container so we can
    // construct it from scratch it at the beginning of each new message.
    boost::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> parser_;

public:
    /**
     * Take ownership of the socket
     * @param socket
     */
    HttpSession(boost::asio::ip::tcp::socket &&socket);

    /**
     * Start the session
     */
    void run();

private:
    void do_read();

    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);

    void on_write(bool close, boost::beast::error_code ec, std::size_t bytes_transferred);

    void do_close();

    /**
     * This function produces an HTTP response for the given
     * request. The type of the response object depends on the
     * contents of the request, so the interface requires the
     * caller to pass a generic lambda for receiving the response.
     * @param req
     * @param send
     */
    void handle_request(boost::beast::http::request<boost::beast::http::string_body> &&req, HttpSession::PipelineQueue& send);

};



