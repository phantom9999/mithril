#pragma once

#include <boost/fiber/future.hpp>

namespace fiber {
template<typename Type>
using Promise = boost::fibers::promise<Type>;

template<typename Type>
using Future = boost::fibers::future<Type>;
}
