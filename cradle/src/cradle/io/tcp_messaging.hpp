#ifndef CRADLE_IO_TCP_IO_HPP
#define CRADLE_IO_TCP_IO_HPP

// This file implements the general pattern used within Astroid to transmit
// messages over TCP.

#include <cstring>
#include <queue>

#include <boost/shared_array.hpp>
#include <boost/static_assert.hpp>

#include <cradle/io/generic_io.hpp>
#include <cradle/io/raw_memory_io.hpp>
#include <cradle/endian.hpp>

// Boost ASIO complains if we don't define this.
#ifdef WIN32
    #define _WIN32_WINNT 0x0501 // Windows XP
#endif
#include <boost/asio.hpp>

namespace cradle {

using boost::asio::ip::tcp;

struct message_header
{
    uint8_t ipc_version;
    uint8_t reserved_a;
    uint8_t code;
    uint8_t reserved_b;
    uint64_t body_length;
};

size_t const ipc_message_header_size = 12;

// Deserialize the message header from the given buffer.
// Buffer must contain (at least) ipc_message_header_size bytes.
message_header static
deserialize_message_header(uint8_t const* buffer)
{
    message_header header;
    raw_memory_reader reader(buffer, ipc_message_header_size);
    header.ipc_version = read_int<uint8_t>(reader);
    header.reserved_a = read_int<uint8_t>(reader);
    header.code = read_int<uint8_t>(reader);
    header.reserved_b = read_int<uint8_t>(reader);
    header.body_length = read_int<uint64_t>(reader);
    return header;
}

// Serialize the given message header into a buffer for transmission.
byte_vector static
serialize_message_header(message_header const& header)
{
    byte_vector buffer;
    raw_memory_writer writer(buffer);
    write_int(writer, header.ipc_version);
    write_int(writer, header.reserved_a);
    write_int(writer, header.code);
    write_int(writer, header.reserved_b);
    write_int(writer, header.body_length);
    assert(buffer.size() == ipc_message_header_size);
    return buffer;
}

// Read a message from the given socket.
template<class IncomingMessage>
IncomingMessage
read_message(
    tcp::socket& socket,
    uint8_t ipc_version)
{
    // Read the header.
    boost::shared_array<uint8_t>
        header_buffer(new uint8_t[ipc_message_header_size]);
    boost::asio::read(socket,
        boost::asio::buffer(header_buffer.get(), ipc_message_header_size));
    auto header = deserialize_message_header(header_buffer.get());
    if (header.ipc_version != ipc_version)
        throw exception("IPC version doesn't match");

    // Read the body.
    boost::shared_array<uint8_t>
        body_buffer(new uint8_t[header.body_length]);
    boost::asio::read(socket,
        boost::asio::buffer(body_buffer.get(), header.body_length));
    IncomingMessage message;
    read_message_body(&message, header.code, body_buffer, header.body_length);
    return message;
}

// Write a message to the given socket.
template<class OutgoingMessage>
void
write_message(
    tcp::socket& socket,
    uint8_t ipc_version,
    OutgoingMessage const& message)
{
    // Write the header.
    message_header header;
    header.ipc_version = ipc_version;
    header.reserved_a = 0;
    header.code = uint8_t(get_message_code(message));
    header.reserved_b = 0;
    header.body_length = uint64_t(get_message_body_size(message));
    auto buffer = serialize_message_header(header);
    boost::asio::write(socket,
        boost::asio::buffer(&buffer[0], buffer.size()));

    // Write the body.
    write_message_body(socket, message);
}

}

#endif
