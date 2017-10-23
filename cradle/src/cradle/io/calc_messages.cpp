#include <cradle/io/calc_messages.hpp>

#include <cradle/io/msgpack_io.hpp>
#include <cradle/io/raw_memory_io.hpp>

namespace cradle {

// SERVER MESSAGES

void
read_message_body(
    calc_supervisor_message* message,
    uint8_t code,
    boost::shared_array<uint8_t> const& body,
    size_t length)
{
    raw_memory_reader reader(body.get(), length);
    switch (static_cast<calc_message_code>(code))
    {
     case calc_message_code::FUNCTION:
      {
        calc_supervisor_calculation_request request;
        request.name = read_string<uint8_t>(reader);
        auto n_args = read_int<uint16_t>(reader);
        request.args.resize(n_args);
        // Allow the arguments to claim ownership of the buffer in case they
        // want to reference data directly from it.
        ownership_holder ownership(body);
        for (uint16_t i = 0; i != n_args; ++i)
        {
            auto arg_length = read_int<uint64_t>(reader);
            parse_msgpack_value(
                &request.args[i],
                ownership,
                reader.buffer,
                arg_length);
            advance(reader, arg_length);
        }
        set_to_function(*message, std::move(request));
        break;
      }
     case calc_message_code::PING:
        *message =
            make_calc_supervisor_message_with_ping(read_string(reader, 32));
        break;
     default:
        throw exception("unrecognized IPC message code");
    }
}

// PROVIDER MESSAGES

calc_message_code
get_message_code(calc_provider_message const& message)
{
    switch (message.type)
    {
     case calc_provider_message_type::REGISTRATION:
        return calc_message_code::REGISTER;
     case calc_provider_message_type::PONG:
        return calc_message_code::PONG;
     case calc_provider_message_type::PROGRESS:
        return calc_message_code::PROGRESS;
     case calc_provider_message_type::RESULT:
        return calc_message_code::RESULT;
     case calc_provider_message_type::FAILURE:
        return calc_message_code::FAILURE;
     default:
        throw exception("invalid calc provider message type");
    }
}

byte_vector static
serialize_message(calc_provider_message const& message)
{
    byte_vector buffer;
    raw_memory_writer writer(buffer);
    switch (message.type)
    {
     case calc_provider_message_type::REGISTRATION:
      {
        auto pid = as_registration(message);
        write_int(writer, uint16_t(0));
        write_string_contents(writer, pid);
        break;
      }
     case calc_provider_message_type::PONG:
      {
        auto code = as_pong(message);
        write_string_contents(writer, code);
        break;
      }
     case calc_provider_message_type::PROGRESS:
      {
        auto progress = as_progress(message);
        write_float(writer, progress.value);
        write_string<uint16_t>(writer, progress.message);
        break;
      }
     case calc_provider_message_type::RESULT:
        // This could be very large, so we don't ever want to keep the whole
        // serialized form in memory.
        throw exception("cannot write calc provider RESULT message to buffer");
     case calc_provider_message_type::FAILURE:
      {
        auto failure = as_failure(message);
        write_string<uint8_t>(writer, failure.code);
        write_string<uint16_t>(writer, failure.message);
        break;
      }
     default:
        throw exception("invalid calc provider message type");
    }
    return buffer;
}

// This can be used as a buffer for the msgpack-c library and will simply
// count the number of bytes that passes through it.
struct msgpack_counting_buffer
{
    size_t size = 0;

    void write(const char* data, size_t size)
    {
        this->size += size;
    }
};

size_t
get_message_body_size(calc_provider_message const& message)
{
    if (message.type == calc_provider_message_type::RESULT)
    {
        auto const& result = as_result(message);
        // This could be very large, so don't actually write anything.
        msgpack_counting_buffer buffer;
        msgpack::packer<msgpack_counting_buffer> packer(buffer);
        write_msgpack_value(packer, result);
        return buffer.size;
    }
    else
    {
        // Other message are small, so we can just write them out and
        // determine their size.
        return serialize_message(message).size();
    }
}

// This can be used as a buffer for the msgpack-c library and will stream
// anything it receives over a Boost Asio socket (synchronously).
struct msgpack_asio_buffer
{
    msgpack_asio_buffer(tcp::socket& socket)
      : socket_(socket)
    {}

    void write(const char* data, size_t size)
    {
        boost::asio::write(
            socket_,
            boost::asio::buffer(data, size));
    }

    tcp::socket& socket_;
};

void static
stream_value(tcp::socket& socket, value const& x)
{
    msgpack_asio_buffer buffer(socket);
    msgpack::packer<msgpack_asio_buffer> packer(buffer);
    write_msgpack_value(packer, x);
}

void
write_message_body(
    tcp::socket& socket,
    calc_provider_message const& message)
{
    if (message.type == calc_provider_message_type::RESULT)
    {
        // This could be very large, so we handle this one in a custom
        // manner that allows streaming.
        stream_value(socket, as_result(message));
    }
    else
    {
        // Other messages are small, so just serialize them to a buffer and
        // write that buffer out all at once.
        auto buffer = serialize_message(message);
        boost::asio::write(
            socket,
            boost::asio::buffer(&buffer[0], buffer.size()));
    }
}

}
