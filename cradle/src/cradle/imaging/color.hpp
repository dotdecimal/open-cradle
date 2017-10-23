#ifndef CRADLE_IMAGING_COLOR_HPP
#define CRADLE_IMAGING_COLOR_HPP

#include <cradle/imaging/channel.hpp>
#include <cradle/math/common.hpp>
#include <cradle/math/interpolate.hpp>
#include <alia/color.hpp>

namespace cradle {

// RGB

using alia::rgb;
using alia::rgb8;
typedef rgb<uint16_t> rgb16;

}

namespace alia {

cradle::raw_type_info get_type_info(rgb8 const& x);
cradle::raw_type_info get_proper_type_info(rgb8 const& x);
template<class T>
void to_value(cradle::value* v, rgb<T> const& x)
{
    cradle::value_map r;
    to_value(&r[cradle::value("r")], x.r);
    to_value(&r[cradle::value("g")], x.g);
    to_value(&r[cradle::value("b")], x.b);
    set(*v, r);
}
template<class T>
void from_value(rgb<T>* x, cradle::value const& v)
{
    cradle::value_map const& r = cradle::cast<cradle::value_map>(v);
    from_value(&x->r, get_field(r, "r"));
    from_value(&x->g, get_field(r, "g"));
    from_value(&x->b, get_field(r, "b"));
}
template<class T>
size_t deep_sizeof(rgb<T>)
{
    using cradle::deep_sizeof;
    return deep_sizeof(T()) * 3;
}
// comparison
template<class T>
bool operator==(rgb<T> const& a, rgb<T> const& b)
{ return a.r == b.r && a.g == b.g && a.b == b.b; }
template<class T>
bool operator!=(rgb<T> const& a, rgb<T> const& b)
{ return !(a == b); }
template<class T>
bool operator<(rgb<T> const& a, rgb<T> const& b)
{
    if (a.r < b.r)
        return true;
    if (b.r < a.r)
        return false;
    if (a.g < b.g)
        return true;
    if (b.g < a.g)
        return false;
    if (a.b < b.b)
        return true;
    if (b.b < a.b)
        return false;
    return false;
}
// interpolation, scaling
template<class T, class F>
rgb<T> interpolate(rgb<T> const& a, rgb<T> const& b, F f)
{
    return rgb<T>(
        cradle::interpolate(a.r, b.r, f),
        cradle::interpolate(a.g, b.g, f),
        cradle::interpolate(a.b, b.b, f));
}
template<class T, class F>
rgb<T> scale(rgb<T> const& a, F f)
{ return interpolate(rgb<T>(0, 0, 0), a, f); }
// componentwise +
template<class T>
rgb<T> operator+(rgb<T> const& a, rgb<T> const& b)
{ return rgb<T>(a.r + b.r, a.g + b.g, a.b + b.b); }
template<class T>
rgb<T>& operator+=(rgb<T>& a, rgb<T> const& b)
{
    a.r += b.r;
    a.g += b.g;
    a.b += b.b;
    return a;
}
// componentwise -
template<class T>
rgb<T> operator-(rgb<T> const& a, rgb<T> const& b)
{ return rgb<T>(a.r - b.r, a.g - b.g, a.b - b.b); }
template<class T>
rgb<T>& operator-=(rgb<T>& a, rgb<T> const& b)
{
    a.r -= b.r;
    a.g -= b.g;
    a.b -= b.b;
    return a;
}
template<class MappingT, class T>
rgb<MappingT> apply_linear_function(
    cradle::linear_function<MappingT> const& mapping, rgb<T> const& p)
{
    return rgb<MappingT>(mapping(p.r), mapping(p.g), mapping(p.b));
}
template<class T, class Value>
void fill_channels(rgb<T>& p, Value v)
{
    p.r = v;
    p.g = v;
    p.b = v;
}

}

namespace cradle {

// metafunctions, etc
template<class T>
struct pixel_channel_type<rgb<T> >
{
    typedef T type;
};
template<class T, class NewChannelT>
struct replace_channel_type<rgb<T>,NewChannelT>
{
    typedef rgb<NewChannelT> type;
};
template<class DstT, class SrcT>
struct channel_converter<rgb<DstT>,rgb<SrcT> >
{
    static rgb<DstT> apply(rgb<SrcT> const& src)
    {
        rgb<DstT> dst;
        dst.r = channel_convert<DstT>(src.r);
        dst.g = channel_convert<DstT>(src.g);
        dst.b = channel_convert<DstT>(src.b);
        return dst;
    }
};

// RGBA

using alia::rgba;
using alia::rgba8;
typedef rgba<uint16_t> rgba16;

// Given a color palette and a list of colors that are already in use, select
// a new color to add to the group that will stand out.
rgb8
choose_new_color(
    std::vector<rgb8> const& palette,
    std::vector<rgb8> const& already_in_use);

}

namespace alia {

cradle::raw_type_info get_type_info(rgba8 const& x);
cradle::raw_type_info get_proper_type_info(rgba8 const& x);
template<class T>
void to_value(cradle::value* v, rgba<T> const& x)
{
    cradle::value_map r;
    to_value(&r[cradle::value("r")], x.r);
    to_value(&r[cradle::value("g")], x.g);
    to_value(&r[cradle::value("b")], x.b);
    to_value(&r[cradle::value("a")], x.a);
    set(*v, r);
}
template<class T>
void from_value(rgba<T>* x, cradle::value const& v)
{
    cradle::value_map const& r = cradle::cast<cradle::value_map>(v);
    from_value(&x->r, get_field(r, "r"));
    from_value(&x->g, get_field(r, "g"));
    from_value(&x->b, get_field(r, "b"));
    from_value(&x->a, get_field(r, "a"));
}
template<class T>
size_t deep_sizeof(rgba<T>)
{
    using cradle::deep_sizeof;
    return deep_sizeof(T()) * 4;
}
// comparison
template<class T>
bool operator==(rgba<T> const& a, rgba<T> const& b)
{ return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }
template<class T>
bool operator!=(rgba<T> const& a, rgba<T> const& b)
{ return !(a == b); }
template<class T>
bool operator<(rgba<T> const& a, rgba<T> const& b)
{
    if (a.r < b.r)
        return true;
    if (b.r < a.r)
        return false;
    if (a.g < b.g)
        return true;
    if (b.g < a.g)
        return false;
    if (a.b < b.b)
        return true;
    if (b.b < a.b)
        return false;
    if (a.a < b.a)
        return true;
    if (b.a < a.a)
        return false;
    return false;
}
// interpolation, scaling
template<class T, class F>
rgba<T> interpolate(rgba<T> const& a, rgba<T> const& b, F f)
{
    return rgba<T>(
        cradle::interpolate(a.r, b.r, f),
        cradle::interpolate(a.g, b.g, f),
        cradle::interpolate(a.b, b.b, f),
        cradle::interpolate(a.a, b.a, f));
}
template<class T, class F>
rgba<T> scale(rgba<T> const& a, F f)
{ return interpolate(rgba<T>(0, 0, 0, a.a), a, f); }
// componentwise +
template<class T>
rgba<T> operator+(rgba<T> const& a, rgba<T> const& b)
{ return rgba<T>(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a); }
template<class T>
rgba<T>& operator+=(rgba<T>& a, rgba<T> const& b)
{
    a.r += b.r;
    a.g += b.g;
    a.b += b.b;
    a.a += b.a;
    return a;
}
// componentwise -
template<class T>
rgba<T> operator-(rgba<T> const& a, rgba<T> const& b)
{ return rgba<T>(a.r - b.r, a.g - b.g, a.b - b.b, a.a - b.a); }
template<class T>
rgba<T>& operator-=(rgba<T>& a, rgba<T> const& b)
{
    a.r -= b.r;
    a.g -= b.g;
    a.b -= b.b;
    a.a -= b.a;
    return a;
}
template<class MappingT, class T>
rgba<MappingT> apply_linear_function(
    cradle::linear_function<MappingT> const& mapping, rgba<T> const& p)
{
    return rgba<MappingT>(mapping(p.r), mapping(p.g), mapping(p.b),
        mapping(p.a));
}
template<class T, class Value>
void fill_channels(rgba<T>& p, Value v)
{
    p.r = v;
    p.g = v;
    p.b = v;
    p.a = v;
}

}

namespace cradle {

// metafunctions, etc
template<class T>
struct pixel_channel_type<rgba<T> >
{
    typedef T type;
};
template<class T, class NewChannelT>
struct replace_channel_type<rgba<T>,NewChannelT>
{
    typedef rgba<NewChannelT> type;
};
template<class DstT, class SrcT>
struct channel_converter<rgba<DstT>,rgba<SrcT> >
{
    static rgba<DstT> apply(rgba<SrcT> const& src)
    {
        rgba<DstT> dst;
        dst.r = channel_convert<DstT>(src.r);
        dst.g = channel_convert<DstT>(src.g);
        dst.b = channel_convert<DstT>(src.b);
        dst.a = channel_convert<DstT>(src.a);
        return dst;
    }
};

}

#endif
