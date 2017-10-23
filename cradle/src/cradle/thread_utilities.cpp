#include <cradle/thread_utilities.hpp>

// LOWER_THREAD_PRIORITY

#ifdef WIN32

#include <windows.h>

namespace cradle {

void lower_thread_priority(boost::thread& thread)
{
    HANDLE win32_thread = thread.native_handle();
    SetThreadPriority(win32_thread, THREAD_PRIORITY_BELOW_NORMAL);
}

}

#else

// ignore warnings for GCC
#pragma GCC diagnostic ignored "-Wunused-parameter"

namespace cradle {

void lower_thread_priority(boost::thread& thread)
{
}

}

#endif
