#include <boost/asio/bind_executor.hpp>
#include <boost/asio/signal_set.hpp>
#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>
#include "http_listener.h"

int main(int argc, char *argv[]) {
    auto const address = boost::asio::ip::make_address("0.0.0.0");
    auto const port = 8080;
    auto const threads = 8;

    // The io_context is required for all I/O
    boost::asio::io_context ioc{threads};

    // Create and launch a listening port
    std::make_shared<HttpListener>(ioc, boost::asio::ip::tcp::endpoint{address, port})->run();

    // Capture SIGINT and SIGTERM to perform a clean shutdown
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&](boost::beast::error_code const &, int) {
                // Stop the `io_context`. This will cause `run()`
                // to return immediately, eventually destroying the
                // `io_context` and all of the sockets in it.
                ioc.stop();
            });

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
        v.emplace_back(
                [&ioc] {
                    ioc.run();
                });
    ioc.run();

    // (If we get here, it means we got a SIGINT or SIGTERM)

    // Block until all the threads exit
    for (auto &t: v)
        t.join();

    return EXIT_SUCCESS;
}

