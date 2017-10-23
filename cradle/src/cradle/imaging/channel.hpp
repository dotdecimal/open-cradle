#ifndef CRADLE_IMAGING_CHANNEL_HPP
#define CRADLE_IMAGING_CHANNEL_HPP

#include <cradle/common.hpp>
#include <cmath>

namespace cradle {

// A metafunction for obtaining a pixel's channel type.
template<typename T>
struct pixel_channel_type
{
    typedef T type;
};

// A metafunction for obtaining the unsigned version of a channel type.
template<typename T>
struct unsigned_channel_type
{
};
template<typename T>
struct unsigned_channel_type<T const>
{
    typedef typename unsigned_channel_type<T>::type const type;
};
template<>
struct unsigned_channel_type<uint8_t>
{
    typedef uint8_t type;
};
template<>
struct unsigned_channel_type<int8_t>
{
    typedef uint8_t type;
};
template<>
struct unsigned_channel_type<uint16_t>
{
    typedef uint16_t type;
};
template<>
struct unsigned_channel_type<int16_t>
{
    typedef uint16_t type;
};
template<>
struct unsigned_channel_type<uint32_t>
{
    typedef uint32_t type;
};
template<>
struct unsigned_channel_type<int32_t>
{
    typedef uint32_t type;
};

// A metafunction for obtaining a type that can be used for summing up values
// of another type.
template<typename T>
struct sum_type
{
    typedef T type;
};
template<typename T>
struct sum_type<T const>
{
    typedef typename sum_type<T>::type type;
};
template<>
struct sum_type<uint8_t>
{
    typedef unsigned type;
};
template<>
struct sum_type<int8_t>
{
    typedef int type;
};
template<>
struct sum_type<uint16_t>
{
    typedef unsigned type;
};
template<>
struct sum_type<int16_t>
{
    typedef int type;
};
template<>
struct sum_type<uint32_t>
{
    typedef uint64_t type;
};
template<>
struct sum_type<int32_t>
{
    typedef int64_t type;
};

// A metafunction for replacing the channel type in a pixel type.
// The default implementation is for pixel types that are simply channel types.
template<class T, class NewChannelT>
struct replace_channel_type
{
    typedef NewChannelT type;
};

// casting
template<class DstT, class SrcT>
struct channel_caster
{
};
template<class T>
struct channel_caster<T,T>
{
    static T apply(T value) { return value; }
};
template<>
struct channel_caster<double,double>
{
    static double apply(double value) { return value; }
};
template<>
struct channel_caster<float,float>
{
    static float apply(float value) { return value; }
};
template<class T>
struct channel_caster<double,T>
{
    static double apply(T value) { return static_cast<double>(value); }
};
template<class T>
struct channel_caster<float,T>
{
    static double apply(T value) { return static_cast<float>(value); }
};
template<>
struct channel_caster<float,double>
{
    static float apply(double value) { return float(value); }
};
template<>
struct channel_caster<double,float>
{
    static double apply(float value) { return double(value); }
};
template<class T>
struct channel_caster<T,double>
{
    static T apply(double value) { return T(std::floor(value + 0.5)); }
};
template<class T>
struct channel_caster<T,float>
{
    static T apply(float value) { return T(std::floor(value + 0.5)); }
};
template<class DstT, class SrcT>
DstT channel_cast(SrcT value)
{ return channel_caster<DstT,SrcT>::apply(value); }

// converting
template<class DstT, class SrcT>
struct channel_converter
{
    static DstT apply(SrcT value) { return value; }
};
// double -> float
template<>
struct channel_converter<float,double>
{
    static float apply(double value) { return float(value); }
};
// int64_t -> double
template<>
struct channel_converter<double,int64_t>
{
    static double apply(int64_t value) { return double(value); }
};
// uint64_t -> double
template<>
struct channel_converter<double,uint64_t>
{
    static double apply(uint64_t value) { return double(value); }
};

template<class DstT, class SrcT>
DstT channel_convert(SrcT value)
{ return channel_converter<DstT,SrcT>::apply(value); }

// multiplication
static inline uint8_t channel_multiply(uint8_t a, uint8_t b)
{
    uint32_t x = unsigned(a) * unsigned(b) + 128;
    return uint8_t((x + (x >> 8)) >> 8);
}
static inline float channel_multiply(float a, float b)
{ return a * b; }
static inline double channel_multiply(double a, double b)
{ return a * b; }

}

#endif
