#ifndef CRADLE_DATE_TIME_HPP
#define CRADLE_DATE_TIME_HPP

#include <cradle/common.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace cradle {

typedef boost::gregorian::date date;
typedef boost::posix_time::ptime time;

// Get a string representation of a date.
std::string to_string(date const& d);

// Get a string representation of a date/time.
std::string to_string(time const& t);

// Get a string representation of a date/time adjust to the local time zone.
std::string to_local_string(time const& t);

// Given a date/time, convert it to the local time zone and get a string
// represention of just the date portion.
std::string to_local_date_string(time const& t);

// Given a date/time, convert it to the local time zone and get a string
// represention of just the time portion.
std::string to_local_time_string(time const& t);

struct value;

time parse_time(std::string const& s);
string to_value_string(time const& t);

void to_value(value* v, date const& x);
void from_value(date* x, value const& v);

void to_value(value* v, time const& x);
void from_value(time* x, value const& v);

}

namespace boost { namespace gregorian {

static inline cradle::raw_type_info
get_type_info(date)
{
    return cradle::raw_type_info(cradle::raw_kind::SIMPLE,
        cradle::any(cradle::raw_simple_type::STRING));
}

static inline size_t
deep_sizeof(date)
{
    return sizeof(date);
}

}}

namespace boost { namespace posix_time {

static inline cradle::raw_type_info
get_type_info(ptime)
{
    return cradle::raw_type_info(cradle::raw_kind::SIMPLE,
        cradle::any(cradle::raw_simple_type::DATETIME));
}

static inline size_t
deep_sizeof(ptime)
{
    return sizeof(ptime);
}

}}

namespace std {

template<>
struct hash<cradle::date>
{
    size_t operator()(cradle::date const& x) const
    { return alia::invoke_hash(cradle::to_string(x)); }
};
template<>
struct hash<boost::gregorian::greg_day>
{
    size_t operator()(boost::gregorian::greg_day x) const
    { return size_t(x); }
};
template<>
struct hash<cradle::time>
{
    size_t operator()(cradle::time const& x) const
    { return alia::invoke_hash(cradle::to_string(x)); }
};

// expanded representation of a date
api(struct internal)
struct expanded_date
{
    int year;
    // January is 1
    int month;
    int day;
};

expanded_date expand_date(date const& collapsed);

date collapse_date(expanded_date const& expanded);

}

#endif
