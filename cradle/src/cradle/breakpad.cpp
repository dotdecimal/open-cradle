#include <cradle/breakpad.hpp>

#include <codecvt>
#include <locale>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#pragma warning(push, 0)
#include <client/windows/crash_generation/client_info.h>
#include <client/windows/crash_generation/crash_generation_server.h>
#include <client/windows/handler/exception_handler.h>
#include <client/windows/common/ipc_protocol.h>
#pragma warning(pop)

#include <cradle/io/file.hpp>

#ifdef NDEBUG
    #pragma comment(lib, "breakpad/lib/release/common")
    #pragma comment(lib, "breakpad/lib/release/crash_generation_server")
    #pragma comment(lib, "breakpad/lib/release/crash_generation_client")
    #pragma comment(lib, "breakpad/lib/release/exception_handler")
#else
    #pragma comment(lib, "breakpad/lib/debug/common")
    #pragma comment(lib, "breakpad/lib/debug/crash_generation_server")
    #pragma comment(lib, "breakpad/lib/debug/crash_generation_client")
    #pragma comment(lib, "breakpad/lib/debug/exception_handler")
#endif

namespace cradle {

struct crash_reporting_implementation
{
    alia__shared_ptr<google_breakpad::ExceptionHandler> handler;
    alia__shared_ptr<google_breakpad::CrashGenerationServer> server;
};

void static
start_crash_server(crash_reporting_implementation& impl,
    std::wstring const& crash_dir, std::wstring const& pipe_name)
{
    if (impl.server)
        return;

    impl.server.reset(
        new google_breakpad::CrashGenerationServer(
            pipe_name, NULL, NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, true, &crash_dir));

    if (!impl.server->Start())
    {
        impl.server.reset();
        throw cradle::exception("Unable to start crash reporting server.");
    }
}

std::wstring static
to_wstring(string const& s)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t> > convert;
    return convert.from_bytes(s);
}

void static
begin_crash_reporting(crash_reporting_implementation& impl,
    file_path const& crash_dir, string const& app_id, string const& version)
{
    std::ostringstream pipe_stream;
    boost::uuids::uuid pipe_uuid = boost::uuids::random_generator()();
    pipe_stream << "\\\\.\\pipe\\CRADLE\\" + app_id + "\\" << pipe_uuid;
    auto pipe_name = to_wstring(pipe_stream.str());

    auto wide_app_id = to_wstring(app_id);
    auto wide_version = to_wstring(version);

    static size_t custom_info_count = 2;
    static google_breakpad::CustomInfoEntry custom_info_entries[] = {
        google_breakpad::CustomInfoEntry(L"app", wide_app_id.c_str()),
        google_breakpad::CustomInfoEntry(L"version", wide_version.c_str())
    };
    google_breakpad::CustomClientInfo custom_info =
        { custom_info_entries, custom_info_count};

    auto wide_crash_dir = crash_dir.string<std::wstring>();

    start_crash_server(impl, wide_crash_dir, pipe_name);
    // This is needed for CRT to not show dialog for invalid param
    // failures and instead let the code handle it.
    _CrtSetReportMode(_CRT_ASSERT, 0);
    impl.handler.reset(
        new google_breakpad::ExceptionHandler(
            wide_crash_dir, NULL, NULL, NULL,
            google_breakpad::ExceptionHandler::HANDLER_ALL,
            MiniDumpNormal, pipe_name.c_str(), &custom_info));
}

void crash_reporting_context::begin(
    file_path const& crash_dir, string const& app_id, string const& version)
{
    impl_ = new crash_reporting_implementation;
    begin_crash_reporting(*impl_, crash_dir, app_id, version);
}

void crash_reporting_context::end()
{
    delete impl_;
    impl_ = 0;
}

}
