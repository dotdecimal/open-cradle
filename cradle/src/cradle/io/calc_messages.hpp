#ifndef CRADLE_IO_CALC_MESSAGES_HPP
#define CRADLE_IO_CALC_MESSAGES_HPP

#include <boost/function.hpp>
#include <boost/shared_array.hpp>

#include <cradle/common.hpp>
#include <cradle/io/tcp_messaging.hpp>

namespace cradle {

// The following describe the protocol between the calculation supervisor and
// the calculation provider.

enum class calc_message_code : uint8_t
{
    REGISTER = 0,
    FUNCTION,
    PROGRESS,
    RESULT,
    FAILURE,
    PING,
    PONG
};

// MESSAGES FROM THE SUPERVISOR

api(struct internal)
struct calc_supervisor_calculation_request
{
    string name;
    std::vector<cradle::value> args;
};

struct raw_memory_reader;

api(union internal)
union calc_supervisor_message
{
    calc_supervisor_calculation_request function;
    string ping;
};

// The following interface is required of messages that are going to be read
// from the TCP messaging system.

void
read_message_body(
    calc_supervisor_message* message,
    uint8_t code,
    boost::shared_array<uint8_t> const& body,
    size_t length);

// MESSAGES FROM THE PROVIDER

api(struct internal)
struct calc_provider_progress_update
{
    float value;
    string message;
};

api(struct internal)
struct calc_provider_failure
{
    string code;
    string message;
};

api(union internal)
union calc_provider_message
{
    string registration;
    calc_provider_progress_update progress;
    string pong;
    value result;
    calc_provider_failure failure;
};

// The following interface is required of messages that are to be written out
// through the TCP messaging system.

calc_message_code
get_message_code(calc_provider_message const& message);

size_t
get_message_body_size(calc_provider_message const& message);

void
write_message_body(
    tcp::socket& socket,
    calc_provider_message const& message);

}

#endif
