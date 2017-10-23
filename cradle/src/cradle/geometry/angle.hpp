#ifndef CRADLE_GEOMETRY_ANGLE_HPP
#define CRADLE_GEOMETRY_ANGLE_HPP

#include <cmath>
#include <iostream>
#include <cradle/math/common.hpp>

// This file provides a class for representing angles with compile-time unit
// specification of degrees or radians. Using this means that degrees/radians
// must be explicitly mentioned anywhere that angles are used, and greatly
// reduces the chances of mixing up the two.

namespace cradle {

// angle units
struct radians {};
struct degrees {};

// implementation details
namespace impl {

    template<class T, class From, class To>
    struct angle_conversion {};
    template<class T>
    struct angle_conversion<T,radians,degrees>
    {
        static T apply(T angle)
        { return static_cast<T>(angle * (180 / pi)); }
    };
    template<class T>
    struct angle_conversion<T,degrees,radians>
    {
        static T apply(T angle)
        { return static_cast<T>(angle * (pi / 180)); }
    };
    template<class T, class Units>
    struct angle_conversion<T,Units,Units>
    {
        static T apply(T angle)
        { return angle; }
    };

    template<class T, class Units>
    struct angle_range {};
    template<class T>
    struct angle_range<T,radians>
    {
        static T min_value() { return -pi; }
        static T max_value() { return pi; }
    };
    template<class T>
    struct angle_range<T,degrees>
    {
        static T min_value() { return -180; }
        static T max_value() { return 180; }
    };
}

// angle class
// T is the numeric type used to represent the angle.
// Units is either radians or degrees.
template<typename T, class Units>
class angle
{
 public:
    angle() {}

    // explicit construction from scalar values
    // (This is explicit to avoid mismatches between scalar and units.)
    explicit angle(T a) : angle_(a) {}

    // implicit conversion from an angle with different units.
    template<class OtherUnits>
    angle(angle<T,OtherUnits> const& other)
      : angle_(impl::angle_conversion<T,OtherUnits,Units>::apply(other.get()))
    {}

    // Get the actual angle value.
    T const& get() const { return angle_; }
    T& get() { return angle_; }

    // Normalize the angle value so that it's as close to 0 as possible while
    // still representing the same angle.
    void normalize()
    {
        T inc = impl::angle_range<T,Units>::max_value() -
            impl::angle_range<T,Units>::min_value();
        if (angle_ <= impl::angle_range<T,Units>::min_value())
        {
            do
                angle_ += inc;
            while (angle_ <= impl::angle_range<T,Units>::min_value());
        }
        else
        {
            while (angle_ > impl::angle_range<T,Units>::max_value())
                angle_ -= inc;
        }
    }

    // Get the normalized value of the angle.
    T normalized() const
    {
        angle tmp = *this;
        tmp.normalize();
        return tmp.get();
    }

    // Assign a new value.
    angle& operator=(T a)
    {
        angle_ = a;
        return *this;
    }

    // scalar *
    angle operator*(T s) const
    { return angle(angle_ * s); }
    angle& operator*=(T s)
    {
        angle_ *= s;
        return *this;
    }
    // scalar /
    angle operator/(T s) const
    { return angle(angle_ / s); }
    angle& operator/=(T s)
    {
        angle_ /= s;
        return *this;
    }

    // unary -
    angle operator-() const
    { return angle(-angle_); }

    // +
    angle operator+(angle x) const
    { return angle(angle_ + x.angle_); }
    angle& operator+=(angle x)
    {
        angle_ += x.angle_;
        return *this;
    }
    // -
    angle operator-(angle x) const
    { return angle(angle_ - x.angle_); }
    angle& operator-=(angle x)
    {
        angle_ -= x.angle_;
        return *this;
    }

    // comparisons
    bool operator==(angle other) const
    { return angle_ == other.angle_; }
    bool operator!=(angle other) const
    { return angle_ != other.angle_; }
    bool operator<(angle other) const
    { return angle_ < other.angle_; }
    bool operator<=(angle other) const
    { return angle_ <= other.angle_; }
    bool operator>(angle other) const
    { return angle_ > other.angle_; }
    bool operator>=(angle other) const
    { return angle_ >= other.angle_; }

 private:
    T angle_;
};


template<typename T, class Units>
raw_type_info get_type_info(angle<T,Units>)
{ return get_type_info(T()); }

template<typename T, class Units>
size_t deep_sizeof(angle<T,Units>)
{ return deep_sizeof(T()); }

// This forces the external specification of angles to be in degrees.
template<typename T, class Units>
void to_value(value* v, angle<T,Units> x)
{ to_value(v, angle<T,degrees>(x).get()); }
template<typename T, class Units>
void from_value(angle<T,Units>* x, value const& v)
{
    T n;
    from_value(&n, v);
    *x = angle<T,degrees>(n);
}

// adding << operator so that angles can be used with input accessors
// (angle output in degrees)
template<typename T, class Units>
std::ostream& operator<<( std::ostream& ostream,
                          const angle<T, Units>& a)
{
    return ostream << (angle<T,degrees>(a).get());
}

} namespace std {
    template<typename T, class Units>
    struct hash<cradle::angle<T,Units> >
    {
        size_t operator()(cradle::angle<T,Units> x) const
        {
            return alia::invoke_hash(x.get());
        }
    };
} namespace cradle {

// trigonometric functions that work with angles
// These will do the appropriate conversion (if necessary) before passing the
// angle to the standard library trigonometric functions.
template<typename T, class Units>
T sin(angle<T,Units> a)
{ return std::sin(angle<T,radians>(a).get()); }
template<typename T, class Units>
T cos(angle<T,Units> a)
{ return std::cos(angle<T,radians>(a).get()); }
template<typename T, class Units>
T tan(angle<T,Units> a)
{ return std::tan(angle<T,radians>(a).get()); }

}

#endif
