#include <cradle/io/calc_provider.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include <cradle/io/calc_messages.hpp>
#include <cradle/io/tcp_messaging.hpp>

#ifndef _WIN32

#include <unistd.h>
#include <ios>
#include <iostream>
#include <fstream>
#include <string>

//////////////////////////////////////////////////////////////////////////////
//
// process_mem_usage(double &, double &) - takes two doubles by reference,
// attempts to read the system-dependent data for a process' virtual memory
// size and resident set size, and return the results in KB.
//
// On failure, returns 0.0, 0.0

//void static
//process_mem_usage(double& vm_usage, double& resident_set)
//{
//   using std::ios_base;
//   using std::ifstream;
//   using std::string;
//
//   vm_usage     = 0.0;
//   resident_set = 0.0;
//
//   // 'file' stat seems to give the most reliable results
//   //
//   ifstream stat_stream("/proc/self/stat",ios_base::in);
//
//   // dummy vars for leading entries in stat that we don't care about
//   //
//   string pid, comm, state, ppid, pgrp, session, tty_nr;
//   string tpgid, flags, minflt, cminflt, majflt, cmajflt;
//   string utime, stime, cutime, cstime, priority, nice;
//   string O, itrealvalue, starttime;
//
//   // the two fields we want
//   //
//   unsigned long vsize;
//   long rss;
//
//   stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
//               >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
//               >> utime >> stime >> cutime >> cstime >> priority >> nice
//               >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest
//
//   stat_stream.close();
//
//   long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
//   vm_usage     = vsize / 1024.0;
//   resident_set = rss * page_size_kb;
//}

#else

//void static
//process_mem_usage(double& vm_usage, double& resident_set)
//{
//    vm_usage = 0;
//    resident_set = 0;
//}

#endif

namespace cradle {

uint8_t static const ipc_version = 1;

// Check if an error occurred, and if so, throw an appropriate exception.
void static
check_error(boost::system::error_code const& error)
{
    if (error)
        throw exception(error.message());
}

struct calc_provider
{
    boost::asio::io_service io_service;
    tcp::socket socket;

    calc_provider()
      : socket(io_service)
    {}
};

// Does the calc provider have an incoming message?
// (This is a non-blocking poll to check for incoming data on the socket.)
bool static
has_incoming_message(calc_provider& provider)
{
    // Start a reactor-style read operation by providing a null_buffer.
    provider.socket.async_receive(
        boost::asio::null_buffers(),
        [ ](
          boost::system::error_code const& error,
          std::size_t bytes_transferred)
        {
            check_error(error);
        });

    // Poll the I/O service and see if the handler runs.
    bool ready_to_read = provider.io_service.poll() != 0;

    // The I/O service needs to be reset when we do stuff like this.
    provider.io_service.reset();

    return ready_to_read;
}

// internal_message_queue is used to transmit messages from the thread that's
// executing the calculation to the IPC thread.
struct internal_message_queue
{
    std::queue<calc_provider_message> messages;
    boost::mutex mutex;
    // for signaling when new messages arrive in the queue
    boost::condition_variable cv;
};

// Post a message to an internal_message_queue.
void static
post_message(
    internal_message_queue& queue,
    calc_provider_message&& message)
{
    boost::mutex::scoped_lock lock(queue.mutex);
    queue.messages.emplace(message);
    queue.cv.notify_one();
}

struct provider_progress_reporter : progress_reporter_interface
{
    void operator()(float progress)
    {
        post_message(*queue,
            make_calc_provider_message_with_progress(
                calc_provider_progress_update(progress, "")));
    }
    internal_message_queue* queue;
};

// Perform a calculation. (This is executed in a dedicated thread.)
void static
perform_calculation(
    internal_message_queue& queue,
    api_implementation const& api,
    calc_supervisor_calculation_request const& request)
{
    try
    {
        auto const& function = find_function_by_name(api, request.name);

        null_check_in check_in;

        provider_progress_reporter reporter;
        reporter.queue = &queue;

        auto result = function.execute(check_in, reporter, request.args);

        post_message(queue, make_calc_provider_message_with_result(result));
    }
    catch (std::exception& e)
    {
        post_message(
            queue,
            make_calc_provider_message_with_failure(
                calc_provider_failure(
                    "none", // TODO: Implement a system of error codes.
                    e.what())));
    }
}

// Check the internal queue for messages and transmit (and remove) any that
// are found. If it sees a message that indicates that the calculation is
// finished, it returns true.
// Note that the queue's mutex must be locked before calling this.
bool static
transmit_queued_messages(
    tcp::socket& socket,
    internal_message_queue& queue)
{
    while (!queue.messages.empty())
    {
        auto const& message = queue.messages.front();
        write_message(socket, ipc_version, message);
        // If we see either of these, we must be done.
        if (message.type == calc_provider_message_type::RESULT ||
            message.type == calc_provider_message_type::FAILURE)
        {
            return true;
        }
        queue.messages.pop();
    }
    return false;
}

void static
dispatch_and_monitor_calculation(
    calc_provider& provider,
    api_implementation const& api,
    calc_supervisor_calculation_request const& request)
{
    internal_message_queue queue;

    // Dispatch a thread to perform the calculation.
    boost::thread
        thread{
            [&]()
            {
                perform_calculation(queue, api, request);
            }};

    // Monitor for messages from the calculation thread or the supervisor.
    // Note that it's difficult to have this thread wait on messages from both
    // the calculation and the supervisor, so it properly waits on the former
    // and periodically checks the latter. This means that it will pass along
    // results and progress reports almost immediately, but it will be a bit
    // delayed in responding to pings.
    while (true)
    {
        // Forward along messages that come from the calculation thread.
        {
            boost::mutex::scoped_lock lock(queue.mutex);
            // First transmit any messages that got queued while we were in
            // other parts of the loop.
            // (We may have missed the signal for these.)
            if (transmit_queued_messages(provider.socket, queue))
                break;
            // Now spend some time waiting for more messages to arrive.
            if (queue.cv.timed_wait(lock, boost::posix_time::seconds(1)))
            {
                if (transmit_queued_messages(provider.socket, queue))
                    break;
            }
        }

        // Now check for incoming messages on the socket.
        while (has_incoming_message(provider))
        {
            auto message =
                read_message<calc_supervisor_message>(
                    provider.socket,
                    ipc_version);
            switch (message.type)
            {
             case calc_supervisor_message_type::FUNCTION:
                // The supervisor should prevent this from happening.
                assert(0);
                break;
             case calc_supervisor_message_type::PING:
                write_message(
                    provider.socket,
                    ipc_version,
                    make_calc_provider_message_with_pong(as_ping(message)));
                break;
            }
        }
    }

    thread.join();
}

void
provide_calculations(
    int argc, char const* const* argv,
    api_implementation const& api)
{
    auto host = getenv("THINKNODE_HOST");
    auto port = std::stoi(getenv("THINKNODE_PORT"));
    auto pid = getenv("THINKNODE_PID");

    calc_provider provider;

    // Resolve the address of the supervisor.
    tcp::resolver resolver(provider.io_service);
    tcp::resolver::query query(tcp::v4(), host, to_string(port));
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    // Connect to the supervisor and send the registration message.
    boost::asio::connect(provider.socket, endpoint_iterator);
    write_message(
        provider.socket,
        ipc_version,
        make_calc_provider_message_with_registration(pid));

    // Process messages from the supervisor.
    while (1)
    {
        auto message =
            read_message<calc_supervisor_message>(
                provider.socket,
                ipc_version);
        switch (message.type)
        {
         case calc_supervisor_message_type::FUNCTION:
            dispatch_and_monitor_calculation(
                provider,
                api,
                as_function(message));
            break;
         case calc_supervisor_message_type::PING:
            write_message(
                provider.socket,
                ipc_version,
                make_calc_provider_message_with_pong(as_ping(message)));
            break;
        }
    }
}

}
