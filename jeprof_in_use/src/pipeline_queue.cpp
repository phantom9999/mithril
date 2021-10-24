#include "pipeline_queue.h"
#include "http_session.h"
#include <vector>
#include <boost/beast/http.hpp>
#include <boost/make_unique.hpp>
#include <boost/optional.hpp>
#include "utils.h"


struct HttpSession::PipelineQueue::work {
    virtual ~work() = default;

    virtual void operator()() = 0;
};

HttpSession::PipelineQueue::PipelineQueue(HttpSession &self) : self_(self) {
        static_assert(limit > 0, "queue limit must be positive");
        items_.reserve(limit);
}


bool HttpSession::PipelineQueue::isFull() const {
    return items_.size() >= limit;
}

bool HttpSession::PipelineQueue::onWrite() {
    BOOST_ASSERT(!items_.empty());
    auto const was_full = isFull();
    items_.erase(items_.begin());
    if (!items_.empty())
        (*items_.front())();
    return was_full;
}

void HttpSession::PipelineQueue::operator()(boost::beast::http::response<boost::beast::http::string_body> &&msg) {
    struct work_impl : work {
        HttpSession &self_;
        boost::beast::http::response<boost::beast::http::string_body> msg_;

        work_impl(
                HttpSession &self,
                boost::beast::http::response<boost::beast::http::string_body> &&msg)
                : self_(self), msg_(std::move(msg)) {
        }

        void
        operator()() {
            boost::beast::http::async_write(self_.stream_, msg_,
                              boost::beast::bind_front_handler(
                                      &HttpSession::onWrite,
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