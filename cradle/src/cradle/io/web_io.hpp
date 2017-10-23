#ifndef CRADLE_IO_WEB_IO_HPP
#define CRADLE_IO_WEB_IO_HPP

#include <cradle/common.hpp>
#include <cradle/io/file.hpp>

// This file defines a low-level facility for doing authenticated web requests.

namespace cradle {

// Apparently some API is #defining this.
#ifdef DELETE
    #undef DELETE
#endif

// supported web request methods
api(enum internal)
enum class web_request_method
{
    POST,
    GET,
    PUT,
    DELETE
};

// web_request defines the inputs required to make a web request.
api(struct internal)
struct web_request
{
    web_request_method method;
    string url;
    blob body;
    std::vector<string> headers;
};

web_request static inline
make_get_request(string const& url, std::vector<string> const& headers)
{
    return web_request(web_request_method::GET, url, blob(), headers);
}

web_request static inline
make_post_request(string const& url, blob const& data,
    std::vector<string> const& headers)
{
    return web_request(web_request_method::POST, url, data, headers);
}

web_request static inline
make_put_request(string const& url, blob const& data,
    std::vector<string> const& headers)
{
    return web_request(web_request_method::PUT, url, data, headers);
}

web_request static inline
make_delete_request(string const& url, std::vector<string> const& headers)
{
    return web_request(web_request_method::DELETE, url, blob(), headers);
}

std::vector<string> static inline
make_header_list(string const& header)
{
    std::vector<string> headers;
    headers.push_back(header);
    return headers;
}

static std::vector<string> const no_headers;

// web_response defines the output from a successful web request.
api(struct internal)
struct web_response
{
    blob body;
    string headers;
};

// Parse a web_response as a JSON value.
value parse_json_response(web_response const& response);

// Parse a web_response as a MessagePack value.
value parse_msgpack_response(web_response const& response);

// general web-related error
struct web_io_error : exception
{
    web_io_error(string const& message, bool is_transient = true)
      : exception(message), is_transient_(is_transient) {}
    ~web_io_error() throw() {}

    bool is_transient() const { return is_transient_; }

 private:
    bool is_transient_;
};

// a web request failed
struct web_request_failure : web_io_error
{
    web_request_failure(web_request const& request, string const& error,
        long response_code, string const& response_header, bool is_transient);
    ~web_request_failure() throw() {}

    long response_code() const
    { return response_code_; }

    string response_header() const
    { return *response_header_; }

    web_request const& request() const
    { return *request_; }

 private:
    alia__shared_ptr<web_request> request_;
    long response_code_;
    alia__shared_ptr<string> response_header_;
};

// web_io_system provides global initialization and shutdown of the web I/O
// system. Exactly one of these objects must be instantiated by the
// application, and its scope must dominate the scope of all other web I/O
// objects.

struct web_io_system : noncopyable
{
    web_io_system();
    ~web_io_system();
};

// certificate_file sets the path to the CURL certificate file that's used to
// authenticate SSL certificates.
// Note that this is not thread-safe, so you should just call it once at
// startup.
void set_web_certificate_file(file_path const& certificate_file);

// web_connection provides a network connection over which web requests can
// be made.

struct web_connection_impl;

struct web_connection : noncopyable
{
    web_connection();
    ~web_connection();

    web_connection_impl* impl;
};

// web_authentication_credentials stores the user info needed to authenticate
// with the web services.
api(struct internal)
struct web_authentication_credentials
{
    string user;
    string password;
};

// web_session_data is the data necessary to communicate the local session to
// the server.
api(struct internal)
struct web_session_data
{
    string token;
};

// Authenticate with the authentication server.
// The return value includes all cookies returned from the server.
// If response is non-zero, the response from the server will be written to it.
web_session_data
authenticate_web_user(
    web_connection& connection, web_request const& request,
    web_authentication_credentials const& user_info);

// Perform a web request and return the response.
// Since this may take a long time to complete, monitoring is provided.
// Accurate progress reporting relies on the web server providing the size
// of the response.
// 'session' is a list of cookies that will be provided to the server.
web_response
perform_web_request(
    check_in_interface& check_in, progress_reporter_interface& reporter,
    web_connection& connection, web_session_data const& session,
    web_request const& request);

}

#endif
