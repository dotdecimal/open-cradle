#ifndef CRADLE_UNITS_HPP
#define CRADLE_UNITS_HPP

#include <cradle/common.hpp>

#ifndef _WIN32
    // ignore warnings for GCC
    #pragma GCC diagnostic ignored "-Wunused-function"
#endif

// This is the start of a system for checking the units used in calculations.
//
// Note that Boost already includes an extensive system for doing this.
// However, I'm choosing not to use it for various reasons...
// * It's focused on compile-time unit checking whereas Astroid needs
//   considerable runtime flexibility.  For example, it's impossible to
//   represent images that can store either Gray or HU in a single type.
// * To use it properly, a lot of CRADLE would need to be overhauled
//   (e.g., geometric and mathemetical primitives would need to carry units).
//   At the moment, there isn't really time for that.
// * I fear the impact on compilation times.
//
// The idea is to initially implement very simple unit tracking, where
// functions check that input units are correct and record their output units,
// and eventually expand the system towards a runtime version of Boost.Units
// (or perhaps even a compile-time/runtime hybrid).

namespace cradle {

struct units
{
    string name;

    units() {}
    units(char const* name) : name(name) {}
};

static inline string const& get_name(units const& u)
{ return u.name; }

static inline cradle::raw_type_info
get_type_info(units const&)
{
    return cradle::raw_type_info(cradle::raw_kind::SIMPLE,
        any(cradle::raw_simple_type::STRING));
}

static inline size_t
deep_sizeof(units const& x)
{
    return deep_sizeof(x.name);
}

static inline void swap(units& a, units& b)
{ swap(a.name, b.name); }

static inline bool operator==(units const& a, units const& b)
{ return a.name == b.name; }
static inline bool operator!=(units const& a, units const& b)
{ return !(a == b); }
static inline bool operator<(units const& a, units const& b)
{ return a.name < b.name; }

static void to_value(value* v, units const& x)
{ to_value(v, x.name); }
static void from_value(units* x, value const& v)
{ from_value(&x->name, v); }

static std::ostream& operator<<(std::ostream& s, units const& x)
{ s << x.name; return s; }

} namespace std {
    template<>
    struct hash<cradle::units>
    {
        size_t operator()(cradle::units const& x) const
        {
            return alia::invoke_hash(x.name);
        }
    };
} namespace cradle {

// CHECKING

class unit_mismatch : public exception
{
 public:
    unit_mismatch(units const& a, units const& b)
      : cradle::exception("unit mismatch"
            "\n  a: " + get_name(a) +
            "\n  b: " + get_name(b))
      , a_(new units(a))
      , b_(new units(b))
    {}
    ~unit_mismatch() throw() {}

    units const& a() const { return *a_; }
    units const& b() const { return *b_; }

 private:
    alia__shared_ptr<units> a_;
    alia__shared_ptr<units> b_;
};

static inline void check_matching_units(units const& a, units const& b)
{
    if (a != b)
        throw unit_mismatch(a, b);
}

class incorrect_units : public exception
{
 public:
    incorrect_units(units const& expected, units const& actual)
      : cradle::exception("incorrect units"
            "\n  expected: " + get_name(expected) +
            "\n  actual: " + get_name(actual))
      , expected_(new units(expected))
      , actual_(new units(actual))
    {}
    ~incorrect_units() throw() {}

    units const& expected() const { return *expected_; }
    units const& actual() const { return *actual_; }

 private:
    alia__shared_ptr<units> expected_;
    alia__shared_ptr<units> actual_;
};

static inline void check_units(units const& expected, units const& actual)
{
    if (actual != expected)
        throw incorrect_units(expected, actual);
}

// STANDARD UNITS

static units const no_units("");

// length

static units const millimeters("mm");
static units const meters("m");

// dose

static units const gray("Gy");
static units const gray_rbe("Gy(RBE)");

// other

static units const relative_stopping_power("relative stopping power");
static units const hounsfield_units("HU");

}

#endif
