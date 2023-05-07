#define BOOST_TEST_MODULE torch
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/summary.h>

#include <array>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

BOOST_AUTO_TEST_CASE(test_prometheus) {

  using namespace prometheus;

  // create an http server running on port 8080
  Exposer exposer{"127.0.0.1:8080"};

  // create a metrics registry
  // @note it's the users responsibility to keep the object alive
  auto registry = std::make_shared<Registry>();

  // add a new counter family to the registry (families combine values with the
  // same name, but distinct label dimensions)
  //
  // @note please follow the metric-naming best-practices:
  // https://prometheus.io/docs/practices/naming/
  auto& packet_counter = BuildCounter()
      .Name("observed_packets_total")
      .Help("Number of observed packets")
      .Register(*registry);

  auto& packet_guage = BuildGauge().Name("xx_gauge").Help("xxx").Register(*registry);
  auto& a_guage = packet_guage.Add({{"aa", "bb"}});
  auto& packet_summry = BuildSummary().Name("xx_sum").Help("xxx").Register(*registry);
  auto& a_summay = packet_summry.Add({{"cc", "dd"}}, Summary::Quantiles{{0.5, 0.05}, {0.9, 0.01}, {0.95, 0.005}, {0.99, 0.001}}, std::chrono::seconds{10});

  // add and remember dimensional data, incrementing those is very cheap
  auto& tcp_rx_counter =
      packet_counter.Add({{"protocol", "tcp"}, {"direction", "rx"}});
  auto& tcp_tx_counter =
      packet_counter.Add({{"protocol", "tcp"}, {"direction", "tx"}});
  auto& udp_rx_counter =
      packet_counter.Add({{"protocol", "udp"}, {"direction", "rx"}});
  auto& udp_tx_counter =
      packet_counter.Add({{"protocol", "udp"}, {"direction", "tx"}});

  // add a counter whose dimensional data is not known at compile time
  // nevertheless dimensional values should only occur in low cardinality:
  // https://prometheus.io/docs/practices/naming/#labels
  auto& http_requests_counter = BuildCounter()
      .Name("http_requests_total")
      .Help("Number of HTTP requests")
      .Register(*registry);

  // ask the exposer to scrape the registry on incoming HTTP requests
  exposer.RegisterCollectable(registry);

  for (;;) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    const auto random_value = std::rand();

    if (random_value & 1) tcp_rx_counter.Increment();
    if (random_value & 2) tcp_tx_counter.Increment();
    if (random_value & 4) udp_rx_counter.Increment();
    if (random_value & 8) udp_tx_counter.Increment();
    a_guage.Set(rand() % 1000 + 1000);
    a_summay.Observe(rand() % 1000 + 1000);

    const std::array<std::string, 4> methods = {"GET", "PUT", "POST", "HEAD"};
    auto method = methods.at(random_value % methods.size());
    // dynamically calling Family<T>.Add() works but is slow and should be
    // avoided
    http_requests_counter.Add({{"method", method}}).Increment();
  }
}