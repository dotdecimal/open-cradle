#ifndef CRADLE_MATH_FIXED_HPP
#define CRADLE_MATH_FIXED_HPP

namespace cradle {

// Integer is the integer type used to store the number.
// LargerInteger is a (possibly) larger integer type for doing multiplies and
// divides, which might overflow when done directly in the storage type.
// FractionalBits is the number of bits used to represent the fractional
// component of the number.
template<class Integer, class LargerInteger, unsigned FractionalBits>
class fixed
{
    // internal representation
    Integer value_;

    // private construction from the underlying integer type
    enum tag {};
    fixed(tag _, Integer value)
      : value_(value)
    {}

 public:
    // CONSTRUCTORS

    fixed() {}

    fixed(fixed const& other)
      : value_(other.value_)
    {}

    explicit fixed(float x)
      : value_(Integer(x * float(1 << FractionalBits)))
    {}
    explicit fixed(double x)
      : value_(Integer(x * double(1 << FractionalBits)))
    {}
    explicit fixed(Integer x)
      : value_(x << FractionalBits)
    {}

    // ASSIGNMENT

    fixed& operator=(fixed other)
    { value_ = other.value_; return *this; }

    // CONVERSIONS

    float as_float() const
    { return float(value_) / (1 << FractionalBits); }
    double as_double() const
    { return double(value_) / (1 << FractionalBits); }
    Integer as_integer() const
    { return value_ >> FractionalBits; }

    // ARITHMETIC

    fixed operator-() const { return fixed(tag(), -value_); }

    fixed operator-(fixed other) const
    { return fixed(tag(), value_ - other.value_); }

    fixed operator+(fixed other) const
    { return fixed(tag(), value_ + other.value_); }

    fixed operator*(fixed other) const
    {
        return fixed(tag(), (LargerInteger(value_) *
            LargerInteger(other.value_)) >> FractionalBits);
    }

    fixed operator/(fixed other) const
    {
        return fixed(tag(), (LargerInteger(value_) << FractionalBits) /
            LargerInteger(other.value_));
    }

    fixed& operator-=(fixed other)
    { value_ -= other.value_; return *this; }

    fixed& operator+=(fixed other)
    { value_ += other.value_; return *this; }

    fixed& operator*=(fixed other)
    { *this = *this * other; return *this; }

    fixed& operator/=(fixed other)
    { *this = *this / other; return *this; }

    // COMPARISONS

    bool operator==(fixed other) const
    { return value_ == other.value_; }

    bool operator!=(fixed other) const
    { return value_ != other.value_; }

    bool operator<(fixed other) const
    { return value_ < other.value_; }

    bool operator>(fixed other) const
    { return value_ > other.value_; }

    bool operator<=(fixed other) const
    { return value_ <= other.value_; }

    bool operator>=(fixed other) const
    { return value_ >= other.value_; }
};

}

#endif
