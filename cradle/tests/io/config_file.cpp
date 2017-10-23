#include <cradle/io/config_file.hpp>
#include <cradle/geometry/common.hpp>
#include <boost/format.hpp>

#define BOOST_TEST_MODULE file:::config
#include <cradle/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(read_file_test)
{
    int const n_bad_files = 7;
    for (int i = 0; i < n_bad_files; ++i)
    {
        cradle::impl::config::structure s;
        BOOST_CHECK_THROW(read_file(s, test_data_directory() /
            "/io/nptc/config/" / str(boost::format("bad%i.cfg") % i)),
            cradle::impl::config::syntax_error);
    }

    cradle::impl::config::structure s;
    read_file(s, test_data_directory() / "/io/nptc/config/good0.cfg");
    BOOST_CHECK_EQUAL(
        s.get_option("interface-type", "console single-window multi-window"),
        2);
    BOOST_CHECK_EQUAL(
        s.get<cradle::impl::config::untyped_list>("windows").size(),
        2);

    read_file(s, test_data_directory() /
        "/io/nptc/config/good1.cfg");
    BOOST_CHECK_EQUAL(s.get<double>("real"), 2.1);
    BOOST_CHECK_EQUAL(s.get<int>("integer"), 3);
    BOOST_CHECK_THROW(
        s.get<cradle::impl::config::untyped_list>("windows"),
        cradle::impl::config::missing_variable);
    cradle::impl::config::structure p =
        s.get<cradle::impl::config::structure>("structured-point");
    BOOST_CHECK_EQUAL(p.get<double>("x"), 0);
    BOOST_CHECK_EQUAL(p.get<double>("y"), 1.2);
    BOOST_CHECK_EQUAL(s.get<vector3i>("p"), make_vector<int>(1, 0, 2));
    BOOST_CHECK_EQUAL(s.get<vector2d>("q"), make_vector<double>(1.5, 7));
    BOOST_CHECK_EQUAL(s.get<std::string>("foo"), "bar");
}
