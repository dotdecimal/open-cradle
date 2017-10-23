#include <cradle/gui/app/instance.hpp>

#include <wx/glcanvas.h>
#include <wx/msgdlg.h>
#include <cradle/external/clean.hpp>
#include <json\json.h>

#include <boost/program_options.hpp>

#include <alia/ui/utilities/styling.hpp>

#include <cradle/gui/app/internals.hpp>
#include <cradle/gui/app/top_level_ui.hpp>
#include <cradle/gui/internals.hpp>
#include <cradle/io/file.hpp>

namespace cradle {


void reset_task_groups(app_instance& instance)
{
    instance.task_groups.clear();
    instance.phantom_task_groups.clear();
    push_task_group(instance,
        instance.controller->get_root_task_group_controller());
    clear_data_block(instance.task_stack_ui_block);
}

bool static
initialize_app_instance(app_instance& instance,
    unsigned argc, wxCmdLineArgsArray const& argv)
{
    auto& controller = *instance.controller;

    auto app_info = controller.get_app_info();
    instance.info = app_info;

    instance.selected_page = app_level_page::APP_CONTENTS;

    instance.state_write_back_requested = false;

    shared_app_config shared_config;
    try
    {
        shared_config = read_shared_app_config();
    }
    catch (...)
    {
        shared_config = shared_app_config(none, 100, none);
    }
    instance.shared_config = shared_config;

    instance.crash_reporter.begin(
        shared_config.crash_dir ?
            get(shared_config.crash_dir) :
            file_path("."),
        app_info.thinknode_app_id,
        app_info.local_version_id);

    boost::filesystem::path executable_path((char const*)argv[0].ToUTF8());

    namespace po = boost::program_options;

    po::options_description desc("Supported options");
    desc.add_options()
        ("help", "show help message")
        ("version", "show version information")
        ("style-file", po::value<string>(), "set style file")
        ("realm", po::value<string>(), "specify the realm to use (by ID)")
        ("username", po::value<string>(), "set username for authentication")
        ("password", po::value<string>(), "set password for authentication")
        ("token", po::value<string>(), "set token for authentication")
    ;

    auto additional_arguments = controller.get_app_command_line_arguments();
    for (auto const& argument : additional_arguments)
    {
        //throw exception(string(argument.first));
        desc.add_options()
            (argument.first, po::value<string>(), argument.second)
        ;
    }

    po::positional_options_description p;

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
        options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        // TODO: This doesn't actually end up going to the Windows console.
        std::cout << desc << "\n";
        return false;
    }

    app_config config;
    try
    {
        config = read_app_config(app_info.thinknode_app_id);
    }
    catch (...)
    {
        config =
            app_config(
                none, none,
                regular_app_window_state(
                    none, make_vector<int>(850, 1000), false, true),
                430,
                350,
                make_layout_vector(0,0),
                1.0);
    }
    instance.config.set(config);

    instance.api_url = read_app_config_file("config.txt", "api_url");

    instance.gui_system.reset(new gui_system);
    initialize_gui_system(&*instance.gui_system,
        shared_config.cache_dir ?
            get(shared_config.cache_dir) :
            get_default_cache_dir("Astroid2"),
        "",
        shared_config.cache_size * int64_t(0x40000000),
        file_path("ca-bundle.crt"));

    // See if the username is available from the command-line or the config.
    if (vm.count("username"))
    {
        auto username = vm["username"].as<string>();
        instance.username.set(username);
    }
    else if (config.username)
    {
        instance.username.set(get(config.username));
    }

    // If token is provide from command-line use it to login with
    if(vm.count("token"))
    {
        auto token = vm["token"].as<string>();
        instance.token.set(token);
        sign_in_wih_token(instance, vm["token"].as<string>());
    }
    // If a password and username are specified on the command-line, initiate the
    // sign-in process.
    else if (vm.count("password") && vm.count("username"))
    {
        start_sign_in(instance, instance.username.get(), vm["password"].as<string>());
    }

    // See if the realm ID is available from the command-line or the config.
    // And if it is, select it.
    if (vm.count("realm"))
    {
        auto realm_id = vm["realm"].as<string>();
        instance.realm_id.set(realm_id);
    }
    else if (config.realm_id)
    {
        auto realm_id = get(config.realm_id);
        instance.realm_id.set(realm_id);
    }

    if (vm.count("style-file"))
    {
        instance.style_file_path = vm["style-file"].as<string>();
    }
    else
    {
        instance.style_file_path = (executable_path.branch_path() /
            "alia.style").string<std::string>();
    }

    // Process any app specific command line arguments
    controller.process_app_command_line_arguments(vm);

    reset_task_groups(instance);

    style_tree_ptr style = parse_style_file(instance.style_file_path.c_str());

    auto* window_controller = new app_window_controller;
    alia__shared_ptr<alia::app_window_controller>
        window_controller_ptr(window_controller);
    window_controller->instance = &instance;

    controller.register_tasks();

    int gl_canvas_attribs[] = {
        WX_GL_RGBA,
        WX_GL_DOUBLEBUFFER,
        WX_GL_STENCIL_SIZE, 1,
        WX_GL_SAMPLE_BUFFERS, 1,
        WX_GL_SAMPLES, 4,
        0 };
    auto* frame =
        create_wx_framed_window(
            app_info.app_name,
            window_controller_ptr, style,
            to_alia(instance.config.get().window_state),
            gl_canvas_attribs);
    #if defined(__WXMSW__)
        frame->SetIcon(wxICON(wxSTD_FRAME));
    #endif

    return true;
}

void static
shut_down_app_instance(app_instance& instance)
{
    clear_all_jobs(*instance.gui_system->bg);
    instance.gui_system.reset();
}

untyped_application::untyped_application(app_controller_interface* controller)
{
    instance = new app_instance;
    instance->controller.reset(controller);

    int attribs[] = { WX_GL_DOUBLEBUFFER, 0 };
    if (!this->InitGLVisual(attribs))
    {
        wxMessageBox("OpenGL not available");
        instance->return_code = -1;
    }
    else
        instance->return_code = 0;
}

untyped_application::~untyped_application()
{
    delete instance;
}

bool untyped_application::OnInit()
{
    try
    {
        return initialize_app_instance(*instance, argc, argv);
    }
    catch (std::exception& e)
    {
        wxMessageBox(std::string("An error occurred during application"
            " initialization.\n\n") + e.what());
        instance->return_code = -1;
    }
    catch (...)
    {
        wxMessageBox("An unknown error occurred during application"
            " initialization.");
        instance->return_code = -1;
    }
    return true;
}

int untyped_application::OnRun()
{
    if (instance->return_code == 0)
        return wxGLApp::OnRun();
    else
        return instance->return_code;
}

int untyped_application::OnExit()
{
    shut_down_app_instance(*instance);

    delete instance;
    instance = 0;

    return wxGLApp::OnExit();
}

}
