#include <cradle/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cradle/common.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/format.hpp>

namespace cradle {

static date parse_date(std::string const& s)
{
    namespace bg = boost::gregorian;
    std::istringstream is(s);
    is.imbue(
        std::locale(std::locale::classic(),
            new bg::date_input_facet("%Y-%m-%d")));
    try
    {
        date d;
        is >> d;
        if (d != date() && !is.fail() &&
            is.peek() == std::istringstream::traits_type::eof())
        {
            return d;
        }
    }
    catch (...)
    {
    }
    throw exception("unrecognized date format");
}

string to_string(date const& d)
{
    namespace bg = boost::gregorian;
    std::ostringstream os;
    os.imbue(
        std::locale(std::cout.getloc(),
            new bg::date_facet("%Y-%m-%d")));
    os << d;
    return os.str();
}

string static
to_value_string(date const& d)
{
    namespace bg = boost::gregorian;
    std::ostringstream os;
    os.imbue(
        std::locale(std::cout.getloc(),
            new bg::date_facet("%Y-%m-%d")));
    os << d;
    return os.str();
}

void to_value(value* v, date const& x)
{
    set(*v, to_value_string(x));
}
void from_value(date* x, value const& v)
{
    *x = parse_date(cast<string>(v));
}

time parse_time(std::string const& s)
{
    namespace bt = boost::posix_time;
    std::istringstream is(s);
    is.imbue(
        std::locale(std::cout.getloc(),
            new bt::time_input_facet("%Y-%m-%dT%H:%M:%s")));
    try
    {
        time t;
        is >> t;
        char z;
        is.get(z);
        if (t != time() && z == 'Z' && !is.fail() &&
            is.peek() == std::istringstream::traits_type::eof())
        {
            return t;
        }
    }
    catch (...)
    {
    }
    throw exception("unrecognized datetime format");
}

string to_string(time const& t)
{
    namespace bt = boost::posix_time;
    std::ostringstream os;
    os.imbue(
        std::locale(std::cout.getloc(),
            new bt::time_facet("%Y-%m-%d %X")));
    os << t;
    return os.str();
}

string to_local_string(time const& t)
{
    auto local = boost::date_time::c_local_adjustor<time>::utc_to_local(t);
    namespace bt = boost::posix_time;
    std::ostringstream os;
    os.imbue(
        std::locale(std::cout.getloc(),
            new bt::time_facet("%Y-%m-%d %X")));
    os << local;
    return os.str();
}

string to_local_date_string(time const& t)
{
    auto local = boost::date_time::c_local_adjustor<time>::utc_to_local(t);
    return to_string(local.date());
}

string to_local_time_string(time const& t)
{
    auto local = boost::date_time::c_local_adjustor<time>::utc_to_local(t);
    namespace bt = boost::posix_time;
    std::ostringstream os;
    os.imbue(
        std::locale(std::cout.getloc(),
            new bt::time_facet("%X")));
    os << local;
    return os.str();
}

string to_value_string(time const& t)
{
    namespace bt = boost::posix_time;
    std::ostringstream os;
    os.imbue(
        std::locale(std::cout.getloc(),
            new bt::time_facet("%Y-%m-%dT%H:%M")));
    os << t;
    // Add the seconds and timezone manually to match Thinknode.
    os << (boost::format(":%02d.%03dZ") % t.time_of_day().seconds()
        % (t.time_of_day().total_milliseconds() % 1000));
    return os.str();
}

void to_value(value* v, time const& x)
{
    set(*v, x);
}
void from_value(time* x, value const& v)
{
    *x = cast<time>(v);
}

expanded_date expand_date(date const& collapsed)
{
    date::ymd_type ymd = collapsed.year_month_day();
    return expanded_date(ymd.year, ymd.month, ymd.day);
}

date collapse_date(expanded_date const& expanded)
{
    return date(expanded.year, boost::gregorian::greg_month(expanded.month),
        expanded.day);
}

}
