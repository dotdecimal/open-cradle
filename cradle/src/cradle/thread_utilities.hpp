#ifndef CRADLE_THREAD_UTILITIES_HPP
#define CRADLE_THREAD_UTILITIES_HPP

#include <boost/thread/thread.hpp>

namespace cradle {

void lower_thread_priority(boost::thread& thread);

}

#endif
