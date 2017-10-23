#include <cradle/io/web_io.hpp>
#include <cradle/io/generic_io.hpp>
#include <cstring>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>

#define CURL_STATICLIB
#include <curl/curl.h>

// Apparently some API has #defined this again since web_io.hpp undid it.
#ifdef DELETE
    #undef DELETE
#endif

namespace cradle {

value parse_json_response(web_response const& response)
{
    return
        parse_json_value(
            reinterpret_cast<char const*>(response.body.data),
            response.body.size);
}

value parse_msgpack_response(web_response const& response)
{
    value v;
    parse_msgpack_value(&v,
        reinterpret_cast<uint8_t const*>(response.body.data),
        response.body.size);
    return v;
}

string static
get_method_name(web_request_method method)
{
    return boost::to_upper_copy(string(get_value_id(method)));
};

string static
format_web_io_error_message(
    web_request const& request,
    long response_code,
    string const& error)
{
    return get_method_name(request.method) + " " + request.url + "\n" +
        to_string(response_code) + "\n" + error;
}

web_request_failure::web_request_failure(
    web_request const& request, string const& error, long response_code,
    string const& response_header, bool is_transient)
  : web_io_error(
        format_web_io_error_message(request, response_code, error),
        is_transient)
  , request_(new web_request(request))
  , response_code_(response_code)
  , response_header_(new string(response_header))
{}

web_io_system::web_io_system()
{
    if (curl_global_init(CURL_GLOBAL_ALL))
        throw web_io_error("web I/O library failed to initialize");
}
web_io_system::~web_io_system()
{
    curl_global_cleanup();
}

static string the_certificate_file;

void set_web_certificate_file(file_path const& certificate_file)
{
    if (!exists(certificate_file))
        throw file_error(certificate_file, "file not found");
    the_certificate_file = certificate_file.string<string>();
}

struct web_connection_impl
{
    CURL* curl;
};

web_connection::web_connection()
{
    impl = new web_connection_impl;

    CURL* curl = curl_easy_init();
    if (!curl)
        throw web_io_error("web I/O library failed to initialize");

    // Enable cookies.
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");

    // Allow requests to be redirected.
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Tell CURL to accept and decode gzipped responses.
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
    curl_easy_setopt(curl, CURLOPT_HTTP_CONTENT_DECODING, 1);

    // Enable SSL verification.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
    curl_easy_setopt(curl, CURLOPT_CAINFO, the_certificate_file.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);

    impl->curl = curl;
}
web_connection::~web_connection()
{
    curl_easy_cleanup(impl->curl);

    delete impl;
}

struct send_transmission_state
{
    char const* data;
    size_t data_length;
    size_t read_position;

    send_transmission_state()
        : data(0), data_length(0), read_position(0)
    {}
};

size_t static
transmit_request_body(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    send_transmission_state& state =
        *reinterpret_cast<send_transmission_state*>(userdata);
    size_t n_bytes =
        (std::min)(size * nmemb, state.data_length - state.read_position);
    if (n_bytes > 0)
    {
        assert(state.data);
        std::memcpy(ptr, state.data + state.read_position, n_bytes);
        state.read_position += n_bytes;
    }
    return n_bytes;
}

struct receive_transmission_state
{
    char* buffer;
    size_t buffer_length;
    size_t write_position;

    receive_transmission_state()
        : buffer(0), buffer_length(0), write_position(0)
    {}
};

size_t static
noop_write_function(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    return size * nmemb;
}

size_t static
record_web_response(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    receive_transmission_state& state =
        *reinterpret_cast<receive_transmission_state*>(userdata);
    if (!state.buffer)
    {
        state.buffer = reinterpret_cast<char*>(malloc(4096));
        if (!state.buffer)
            return 0;
        state.buffer_length = 4096;
        state.write_position = 0;
    }

    // Grow the buffer if necessary.
    size_t n_bytes = size * nmemb;
    if (state.buffer_length < (state.write_position + n_bytes))
    {
        // Each time the buffer grows, it doubles in size.
        // This wastes some memory but should be faster in general.
        size_t new_size = state.buffer_length * 2;
        while (new_size < state.buffer_length + n_bytes)
            new_size *= 2;
        char* new_buffer =
            reinterpret_cast<char*>(realloc(state.buffer, new_size));
        if (!new_buffer)
            return 0;
        state.buffer = new_buffer;
        state.buffer_length = new_size;
    }

    assert(state.buffer);
    std::memcpy(state.buffer + state.write_position, ptr, n_bytes);
    state.write_position += n_bytes;
    return n_bytes;
}

void static
set_up_send_transmission(
    CURL* curl, send_transmission_state& send_state,
    web_request const& request)
{
    send_state.data = reinterpret_cast<char const*>(request.body.data);
    send_state.data_length = request.body.size;
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, transmit_request_body);
    curl_easy_setopt(curl, CURLOPT_READDATA, &send_state);
}

struct curl_progress_data
{
    check_in_interface* check_in;
    progress_reporter_interface* reporter;
};

int static
curl_progress_callback(
    void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    curl_progress_data* data = reinterpret_cast<curl_progress_data*>(clientp);
    try
    {
        (*data->check_in)();
        (*data->reporter)((dltotal + ultotal == 0) ? 0.f :
            float((dlnow + ulnow) / (dltotal + ultotal)));
    }
    catch (...)
    {
        return 1;
    }
    return 0;
}

struct scoped_curl_slist
{
    ~scoped_curl_slist() { curl_slist_free_all(list); }
    curl_slist* list;
};

std::vector<std::string> static
extract_curl_slist_strings(curl_slist* list)
{
    std::vector<std::string> strings;
    for (curl_slist* i = list; i != 0; i = i->next)
        strings.push_back(i->data);
    return strings;
}

blob static
make_blob(receive_transmission_state const& transmission)
{
    blob result;
    result.ownership =
        alia__shared_ptr<char>(transmission.buffer, free);
    result.data = transmission.buffer;
    result.size = transmission.write_position;
    return result;
}

// perform_general_web_request performs a very generalized web request that
// can server as either an authentication request of a normal web request.
void static
perform_general_web_request(
    web_connection& connection, web_request const& request,
    curl_progress_data* progress_data,
    web_authentication_credentials const* auth_info,
    web_session_data const* session,
    std::vector<string> const* input_cookies,
    std::vector<string>* output_cookies,
    web_response* response)
{
    CURL* curl = connection.impl->curl;
    assert(curl);

    // Set the headers for the request.
    scoped_curl_slist headers;
    headers.list = NULL;
    if (session)
    {
        string session_header =
            "Authorization: Bearer " + session->token;
        headers.list = curl_slist_append(headers.list, session_header.c_str());
    }
    for (auto const& header : request.headers)
        headers.list = curl_slist_append(headers.list, header.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers.list);

    // Clear the existing cookies.
    curl_easy_setopt(curl, CURLOPT_COOKIELIST, "ALL");
    // And add the ones for this request.
    if (input_cookies)
    {
        for (auto const& cookie : *input_cookies)
            curl_easy_setopt(curl, CURLOPT_COOKIELIST, cookie.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, request.url.c_str());

    if (auth_info)
    {
        string authentication_string =
            auth_info->user + ":" + auth_info->password;
        curl_easy_setopt(curl, CURLOPT_USERPWD, authentication_string.c_str());
    }

    // Set up for receiving the response.
    receive_transmission_state receive_state;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, record_web_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &receive_state);

    // Set up for receiving the headers.
    receive_transmission_state header_receive_state;
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, record_web_response);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_receive_state);

    // Let CURL know what the method is and set up for sending the body if
    // necessary.
    send_transmission_state send_state;
    if (request.method == web_request_method::PUT)
    {
        set_up_send_transmission(curl, send_state, request);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
        curl_off_t size = request.body.size;
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, size);
    }
    else
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 0);
    if (request.method == web_request_method::POST)
    {
        set_up_send_transmission(curl, send_state, request);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_off_t size = request.body.size;
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, size);
    }
    else
        curl_easy_setopt(curl, CURLOPT_POST, 0);
    if (request.method == web_request_method::DELETE)
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    else
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, 0);

    // If the caller wants to monitor the progress, set that up.
    if (progress_data)
    {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION,
            curl_progress_callback);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, progress_data);
    }
    else
    {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, 0);
    }

    // Perform the request.
    CURLcode result = curl_easy_perform(curl);

    // Check in again here because if the job was canceled inside the above call, it will
    // just look like an error. We need the exception to be rethrown.
    if (progress_data)
    {
        (*progress_data->check_in)();
    }

    // Now check for actual CURL errors.
    if (result != CURLE_OK)
    {
        auto r = curl_easy_strerror(result);
        auto body = to_string(to_value(request.body));
        auto mes = to_string(r) + ":" + body;
        throw web_request_failure(request, mes, 0, string(""), true);
    }

    // Construct the response before checking for errors so that something
    // claims ownership of the receive buffers.
    web_response wr;
    wr.body = make_blob(receive_state);
    blob response_headers = make_blob(header_receive_state);
    wr.headers =
        string(reinterpret_cast<char const*>(response_headers.data),
            response_headers.size);
    if (response)
        *response = wr;

    // Check the response code.
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code != 200)
    {
        // Uncomment this to write the failing request body out to a file.
        // (This can be useful for debugging.)
        //{
        //    std::ofstream f("request_failed.json");
        //    f << value_to_json(to_value(request.body));
        //}
        throw web_request_failure(request,
            string(reinterpret_cast<char const*>(wr.body.data), wr.body.size),
            response_code,
            string(reinterpret_cast<char const*>(response_headers.data),
                response_headers.size),
            false);
    }

    // Record the cookies we got back from the request.
    if (output_cookies)
    {
        scoped_curl_slist cookies;
        result = curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies.list);
        if (result != CURLE_OK)
        {
            throw web_request_failure(request, curl_easy_strerror(result),
                response_code,
                string(reinterpret_cast<char const*>(response_headers.data),
                    response_headers.size),
                true);
        }
        *output_cookies = extract_curl_slist_strings(cookies.list);
    }
}

web_session_data
authenticate_web_user(
    web_connection& connection, web_request const& request,
    web_authentication_credentials const& user_info)
{
    web_response response;
    perform_general_web_request(connection, request, 0, &user_info, 0, 0, 0,
        &response);
    return from_value<web_session_data>(parse_json_response(response));
}

web_response
perform_web_request(
    check_in_interface& check_in, progress_reporter_interface& reporter,
    web_connection& connection, web_session_data const& session,
    web_request const& request)
{
    curl_progress_data progress_data;
    progress_data.check_in = &check_in;
    progress_data.reporter = &reporter;

    web_response response;
    perform_general_web_request(
        connection, request, &progress_data, 0, &session, 0, 0, &response);
    return response;
}

}
