#ifndef CRADLE_IMAGING_VARIANT_HPP
#define CRADLE_IMAGING_VARIANT_HPP

#include <cradle/imaging/image.hpp>
#include <cradle/imaging/color.hpp>
#include <cradle/imaging/geometry.hpp>
#include <cradle/imaging/iterator.hpp>

// Variant images allow you to work with images of varying types without
// knowing the type at compile time. Images can be cast to and from variant
// types of the same dimensionality and storage. Casting from a normal
// image to a variant image erases the compile time type information and
// encodes it in run-time variables.

// A variant image type is created by using the type 'variant' as the Pixel
// type in an image (e.g., image<2,variant,const_view> is a 2D const view of
// some pixels whose type are unknown at compile time).

// This file defines some "standard" pixel formats and channel types. These
// are the types supported by variant images. They can be expanded as needed.

namespace cradle {

// PIXEL FORMATS

// pixel formats supported for variant images
api(enum)
enum class pixel_format
{
    // Grayscale
    GRAY,
    // Red, Blue, Green
    RGB,
    // Red, Blue, Green, Alpha
    RGBA
};

string get_pixel_format_name(pixel_format fmt);

unsigned get_channel_count(pixel_format fmt);

template<class Pixel>
struct pixel_format_of_type
{
    static cradle::pixel_format const value = pixel_format::GRAY;
};
template<class Channel>
struct pixel_format_of_type<rgb<Channel> >
{
    static cradle::pixel_format const value = pixel_format::RGB;
};
template<class Channel>
struct pixel_format_of_type<rgba<Channel> >
{
    static cradle::pixel_format const value = pixel_format::RGBA;
};

// CHANNEL TYPES
api(enum)
enum class channel_type
{
    // int8
    INT8,
    // uint8
    UINT8,
    // int16
    INT16,
    // uint16
    UINT16,
    // int32
    INT32,
    // uint32
    UINT32,
    // int64
    INT64,
    // uint64
    UINT64,
    // float
    FLOAT,
    // double
    DOUBLE
};

string get_channel_type_name(channel_type type);

size_t get_channel_size(channel_type type);

template<class Channel>
struct channel_type_of_type
{
};
template<>
struct channel_type_of_type<int8_t>
{
    static channel_type const value = channel_type::INT8;
};
template<>
struct channel_type_of_type<uint8_t>
{
    static channel_type const value = channel_type::UINT8;
};
template<>
struct channel_type_of_type<int16_t>
{
    static channel_type const value = channel_type::INT16;
};
template<>
struct channel_type_of_type<uint16_t>
{
    static channel_type const value = channel_type::UINT16;
};
template<>
struct channel_type_of_type<int32_t>
{
    static channel_type const value = channel_type::INT32;
};
template<>
struct channel_type_of_type<uint32_t>
{
    static channel_type const value = channel_type::UINT32;
};
template<>
struct channel_type_of_type<int64_t>
{
    static channel_type const value = channel_type::INT64;
};
template<>
struct channel_type_of_type<uint64_t>
{
    static channel_type const value = channel_type::UINT64;
};
template<>
struct channel_type_of_type<float>
{
    static channel_type const value = channel_type::FLOAT;
};
template<>
struct channel_type_of_type<double>
{
    static channel_type const value = channel_type::DOUBLE;
};
template<class Channel>
struct channel_type_of_type<rgb<Channel> >
  : channel_type_of_type<Channel>
{};
template<class Channel>
struct channel_type_of_type<rgba<Channel> >
  : channel_type_of_type<Channel>
{};

// COMBINED TYPE INFO
api(struct)
struct variant_type_info
{
    // pixel formats supported for variant images
    pixel_format format;
    // Channel type supported
    channel_type type;
};

// This error is thrown when you try to interpret a variant image as the wrong
// type.
class image_type_mismatch : public cradle::exception
{
 public:
    image_type_mismatch(
        pixel_format expected_format, channel_type expected_type,
        string const& expected,
        pixel_format actual_format, channel_type actual_type,
        string const& actual)
      : cradle::exception("image format/type mismatch"
              "\n  expected: " + expected
            + "\n  actual: " + actual)
      , expected_type_(expected_type)
      , expected_format_(expected_format)
      , expected_(new string(expected))
      , actual_type_(actual_type)
      , actual_format_(actual_format)
      , actual_(new string(actual))
    {}
    ~image_type_mismatch() throw() {}

    pixel_format get_expected_format() const { return expected_format_; }
    channel_type get_expected_type() const { return expected_type_; }
    string const& get_expected() const { return *expected_; }

    pixel_format get_actual_format() const { return actual_format_; }
    channel_type get_actual_type() const { return actual_type_; }
    string const& get_actual() const { return *actual_; }

 private:
    channel_type expected_type_;
    pixel_format expected_format_;
    alia__shared_ptr<string> expected_;

    channel_type actual_type_;
    pixel_format actual_format_;
    alia__shared_ptr<string> actual_;
};

template<class Pixel>
void set_type_info(variant_type_info& info)
{
    info.format = pixel_format_of_type<Pixel>::value;
    info.type = channel_type_of_type<Pixel>::value;
}

template<class Pixel>
void match_type_info(variant_type_info const& info)
{
    pixel_format expected_format = pixel_format_of_type<Pixel>::value;
    channel_type expected_type = channel_type_of_type<Pixel>::value;
    if (expected_format != info.format || expected_type != info.type)
    {
        throw image_type_mismatch(
            expected_format, expected_type,
            get_channel_type_name(expected_type) + " " +
            get_pixel_format_name(expected_format),
            info.format, info.type,
            get_channel_type_name(info.type) + " " +
            get_pixel_format_name(info.format));
    }
}

template<class Functor>
void dispatch_variant(variant_type_info const& info, Functor& f)
{
    switch (info.format)
    {
     case pixel_format::GRAY:
        switch (info.type)
        {
         case channel_type::INT8:
            f(int8_t());
            break;
         case channel_type::UINT8:
            f(uint8_t());
            break;
         case channel_type::INT16:
            f(int16_t());
            break;
         case channel_type::UINT16:
            f(uint16_t());
            break;
         case channel_type::INT32:
            f(int32_t());
            break;
         case channel_type::UINT32:
            f(uint32_t());
            break;
         case channel_type::INT64:
            f(int64_t());
            break;
         case channel_type::UINT64:
            f(uint64_t());
            break;
         case channel_type::FLOAT:
            f(float());
            break;
         case channel_type::DOUBLE:
            f(double());
            break;
        }
        break;
     case pixel_format::RGB:
        switch (info.type)
        {
         case channel_type::INT8:
            f(rgb<int8_t>());
            break;
         case channel_type::UINT8:
            f(rgb<uint8_t>());
            break;
         case channel_type::INT16:
            f(rgb<int16_t>());
            break;
         case channel_type::UINT16:
            f(rgb<uint16_t>());
            break;
         case channel_type::INT32:
            f(rgb<int32_t>());
            break;
         case channel_type::UINT32:
            f(rgb<uint32_t>());
            break;
         case channel_type::INT64:
            f(rgb<int64_t>());
            break;
         case channel_type::UINT64:
            f(rgb<uint64_t>());
            break;
         case channel_type::FLOAT:
            f(rgb<float>());
            break;
         case channel_type::DOUBLE:
            f(rgb<double>());
            break;
        }
        break;
     case pixel_format::RGBA:
        switch (info.type)
        {
         case channel_type::INT8:
            f(rgba<int8_t>());
            break;
         case channel_type::UINT8:
            f(rgba<uint8_t>());
            break;
         case channel_type::INT16:
            f(rgba<int16_t>());
            break;
         case channel_type::UINT16:
            f(rgba<uint16_t>());
            break;
         case channel_type::INT32:
            f(rgba<int32_t>());
            break;
         case channel_type::UINT32:
            f(rgba<uint32_t>());
            break;
         case channel_type::INT64:
            f(rgba<int64_t>());
            break;
         case channel_type::UINT64:
            f(rgba<uint64_t>());
            break;
         case channel_type::FLOAT:
            f(rgba<float>());
            break;
         case channel_type::DOUBLE:
            f(rgba<double>());
            break;
        }
        break;
    }
}

template<class Functor>
void dispatch_gray_variant(variant_type_info const& info, Functor& f)
{
    if (info.format == pixel_format::GRAY)
    {
        switch (info.type)
        {
         case channel_type::INT8:
            f(int8_t());
            break;
         case channel_type::UINT8:
            f(uint8_t());
            break;
         case channel_type::INT16:
            f(int16_t());
            break;
         case channel_type::UINT16:
            f(uint16_t());
            break;
         case channel_type::INT32:
            f(int32_t());
            break;
         case channel_type::UINT32:
            f(uint32_t());
            break;
         case channel_type::INT64:
            f(int64_t());
            break;
         case channel_type::UINT64:
            f(uint64_t());
            break;
         case channel_type::FLOAT:
            f(float());
            break;
         case channel_type::DOUBLE:
            f(double());
            break;
        }
    }
    else
        throw exception(
            "this function can only be invoked on grayscale images");
}

// STORAGE POLICIES - variant is just a tag that's used to override the
// behavior of the standard storage policies.

// Note that variant is declared in forward.hpp.

// view
struct variant_view_pointer
{
    void* view;
    variant_type_info type_info;
};
static inline void swap(variant_view_pointer& a, variant_view_pointer& b)
{
    using std::swap;
    swap(a.view, b.view);
    swap(a.type_info, b.type_info);
}
static inline bool operator==(variant_view_pointer const& a,
    variant_view_pointer const& b)
{
    return a.view == b.view && a.type_info == b.type_info;
}
static inline bool operator!=(variant_view_pointer const& a,
    variant_view_pointer const& b)
{
    return !(a == b);
}
template<unsigned N>
struct view_pointer_type<N,variant>
{
    typedef variant_view_pointer type;
};
template<class Pixel>
void cast_pointer(variant_view_pointer& dst, Pixel* src)
{
    dst.view = src;
    set_type_info<Pixel>(dst.type_info);
}
template<class Pixel>
void cast_pointer(Pixel*& dst, variant_view_pointer const& src)
{
    match_type_info<Pixel>(src.type_info);
    dst = reinterpret_cast<Pixel*>(src.view);
}
template<class Pixel>
void cast_pointer(variant_view_pointer& dst,
    unique_pointer<Pixel> const& src)
{
    dst.view = src.ptr;
    set_type_info<Pixel>(dst.type_info);
}

// const_view
struct variant_const_view_pointer
{
    void const* view;
    variant_type_info type_info;
};
static inline void swap(variant_const_view_pointer& a,
    variant_const_view_pointer& b)
{
    using std::swap;
    swap(a.view, b.view);
    swap(a.type_info, b.type_info);
}
static inline bool operator==(variant_const_view_pointer const& a,
    variant_const_view_pointer const& b)
{
    return a.view == b.view && a.type_info == b.type_info;
}
static inline bool operator!=(variant_const_view_pointer const& a,
    variant_const_view_pointer const& b)
{
    return !(a == b);
}
template<unsigned N>
struct const_view_pointer_type<N,variant>
{
    typedef variant_const_view_pointer type;
};
template<class Pixel>
void cast_pointer(variant_const_view_pointer& dst, Pixel const* src)
{
    dst.view = src;
    set_type_info<Pixel>(dst.type_info);
}
template<class Pixel>
void cast_pointer(Pixel const*& dst, variant_const_view_pointer const& src)
{
    match_type_info<Pixel>(src.type_info);
    dst = reinterpret_cast<Pixel const*>(src.view);
}
template<class Pixel>
void cast_pointer(variant_const_view_pointer& dst,
    shared_pointer<Pixel> const& src)
{
    dst.view = src.view;
    set_type_info<Pixel>(dst.type_info);
}
static inline void cast_pointer(variant_const_view_pointer& dst,
    variant_view_pointer const& src)
{
    dst.view = src.view;
    dst.type_info = src.type_info;
}
template<class Pixel>
void cast_pointer(variant_const_view_pointer& dst,
    unique_pointer<Pixel> const& src)
{
    dst.view = src.ptr;
    set_type_info<Pixel>(dst.type_info);
}

// other cast_pointer overrides

// simple pointers
template<class Pixel1, class Pixel2>
void cast_pointer(
    Pixel1*& dst,
    Pixel2* src)
{
    dst = (Pixel1*)src;
}
// simple pointers (const)
template<class Pixel1, class Pixel2>
void cast_pointer(
    Pixel1 const*& dst,
    Pixel2 const* src)
{
    dst = (Pixel1 const*)src;
}

// shared
struct variant_shared_pointer
{
    ownership_holder ownership;
    void const* view;
    variant_type_info type_info;
};
static inline bool operator==(variant_shared_pointer const& a,
    variant_shared_pointer const& b)
{
    return a.view == b.view && a.type_info == b.type_info;
}
static inline bool operator!=(variant_shared_pointer const& a,
    variant_shared_pointer const& b)
{
    return !(a == b);
}
template<unsigned N>
struct shared_pointer_type<N,variant>
{
    typedef variant_shared_pointer type;
};
static inline void swap(variant_shared_pointer& a, variant_shared_pointer& b)
{
    using std::swap;
    swap(a.ownership, b.ownership);
    swap(a.view, b.view);
    swap(a.type_info, b.type_info);
}
template<class Pixel>
void cast_pointer(variant_shared_pointer& dst,
    shared_pointer<Pixel> const& src)
{
    dst.ownership = src.ownership;
    dst.view = src.view;
    set_type_info<Pixel>(dst.type_info);
}
template<class Pixel>
void cast_pointer(shared_pointer<Pixel>& dst,
    variant_shared_pointer const& src)
{
    match_type_info<Pixel>(src.type_info);
    dst.ownership = src.ownership;
    dst.view = reinterpret_cast<Pixel const*>(src.view);
}
static inline void cast_pointer(variant_const_view_pointer& dst,
    variant_shared_pointer const& src)
{
    dst.type_info = src.type_info;
    dst.view = src.view;
}
template<class Pixel>
void cast_pointer(Pixel const*& dst,
    variant_shared_pointer const& src)
{
    shared_pointer<Pixel> tmp;
    cast_pointer(tmp, src);
    cast_pointer(dst, tmp);
}

// UTILITY FUNCTIONS

template<unsigned N, class Pixel, class Storage>
image<N,variant,Storage> as_variant(image<N,Pixel,Storage> const& img)
{
    return cast_image<image<N,variant,Storage> >(img);
}

template<class Pixel, unsigned N, class Storage>
image<N,Pixel,Storage> cast_variant(image<N,variant,Storage> const& img)
{
    return cast_image<image<N,Pixel,Storage> >(img);
}

template<unsigned N, class Storage, class Functor>
struct variant_cast_applier
{
    image<N,variant,Storage> const* img;
    Functor* f;
    template<class Pixel>
    void operator()(Pixel)
    { (*f)(cast_variant<Pixel>(*img)); }
};

template<class Functor, unsigned N, class Storage>
void apply_fn_to_variant(Functor& f, image<N,variant,Storage> const& img)
{
    variant_cast_applier<N,Storage,Functor> applier;
    applier.img = &img;
    applier.f = &f;
    dispatch_variant(img.pixels.type_info, applier);
}

template<class Functor, unsigned N, class Storage>
void apply_fn_to_gray_variant(Functor& f, image<N,variant,Storage> const& img)
{
    variant_cast_applier<N,Storage,Functor> applier;
    applier.img = &img;
    applier.f = &f;
    dispatch_gray_variant(img.pixels.type_info, applier);
}

// 1D, 2D, and 3D variant images are considered regular types, so they need
// the standard interface.

CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE(1,cradle::variant)
CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE(2,cradle::variant)
CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE(3,cradle::variant)

// BOXING / UNBOXING

// Gray images have an 'unboxed' form, which stores the pixel array as a
// std::vector of doubles. What's significant about this is that when converted
// to a dynamic value, the pixels are represented a normal list of numbers
// rather than an opaque blob, so they can be easily manipulated.

api(struct with(N:2,3))
template<unsigned N>
struct unboxed_image
{
    vector<N,unsigned> size;
    std::vector<double> pixels;
    vector<N,double> origin;
    c_array<N,vector<N,double> > axes;
};

template<unsigned N>
struct unboxed_pixel_copier
{
    std::vector<double>* dst;
    template<class Pixel, class SP>
    void operator()(image<N,Pixel,SP> const& img)
    {
        auto dst_i = dst->begin();
        auto src_end = get_end(img);
        for (auto src_i = get_begin(img); src_i != src_end; ++src_i, ++dst_i)
            *dst_i = double(*src_i);
    }
};

api(fun with(N:2,3))
template<unsigned N>
unboxed_image<N> unbox_image(image<N,variant,shared> const& boxed)
{
    unboxed_image<N> unboxed;

    unboxed.size = boxed.size;
    unboxed.origin = boxed.origin;
    unboxed.axes = boxed.axes;

    unboxed.pixels.resize(product(unboxed.size));
    unboxed_pixel_copier<N> fn;
    fn.dst = &unboxed.pixels;
    apply_fn_to_gray_variant(fn, boxed);
    for (std::vector<double>::iterator i = unboxed.pixels.begin();
        i != unboxed.pixels.end(); ++i)
    {
        *i = apply(boxed.value_mapping, *i);
    }

    return unboxed;
}

api(fun with(N:2,3))
template<unsigned N>
image<N,variant,shared> box_image(unboxed_image<N> const& unboxed)
{
    image<N,double,unique> img;
    create_image(img, unboxed.size);

    for (unsigned i = 0; i != N; ++i)
    {
        if (unboxed.size[i] <= 0)
            throw exception("box_image: image size must be positive");
    }

    if (unboxed.pixels.size() != product(unboxed.size))
    {
        throw exception(
            "box_image: pixel array size is inconsistent with image size");
    }

    for (size_t i = 0; i != unboxed.pixels.size(); ++i)
        img.pixels.ptr[i] = unboxed.pixels[i];

    img.origin = unboxed.origin;
    img.axes = unboxed.axes;

    return as_variant(share(img));
}
template<unsigned N>
void combine_image_helper(
    int *ii,
    image<N,double,unique> &img,
    int &i,
    std::vector<image<N,double,shared> > const& images,
    int k)
{
    if (k > 0)
    {
        for (ii[k-1] = 0; ii[k-1] < int(img.size[k-1]); ++ii[k-1])
        {
            combine_image_helper<N>(ii, img, i, images, k - 1);
        }
    }
    else
    {
        img.pixels.ptr[i] = 0;
        double pos[N];
        for (unsigned n = 0; n < N; ++n)
        {
            pos[n] = img.origin[n] + (ii[n] + 0.5) * img.axes[n][n];
        }
        double min_dist = 1.0e40;
        double value_at_min = 0.;
        for (unsigned j = 0; j < images.size(); ++j)
        {
            auto image_const_view = as_const_view(images.at(j));
            double d = 0.0;

            unsigned p_i[N];
            for (unsigned n = 0; n < N; ++n)
            {
                p_i[n] = int(std::floor((pos[n] - images.at(j).origin[n]) / (images.at(j).axes[n][n])));
                if (p_i[n] < 0)
                {
                    double dd = images.at(j).origin[n] - pos[n] + 0.5 * images.at(j).axes[n][n];
                    d += dd * dd;
                    p_i[n] = 0;
                }
                else if (p_i[n] >= images.at(j).size[n])
                {
                    double dd = pos[n] - (images.at(j).origin[n] + images.at(j).axes[n][n] * images.at(j).size[n]) + 0.5 * images.at(j).axes[n][n];
                    d += dd * dd;
                    p_i[n] = images.at(j).size[n] - 1;
                }
            }

            int index;
            switch (N)
            {
                case 1:
                    index = p_i[0];
                    break;
                case 2:
                    index = p_i[1] * images.at(j).size[0] + p_i[0];
                    break;
                case 3:
                    index = p_i[2] * images.at(j).size[1] * images.at(j).size[0] + p_i[1] * images.at(j).size[0] + p_i[0];
                    break;
            }

            if (d == 0)
            {
                min_dist = 0.0;
                img.pixels.ptr[i] += double(image_const_view.pixels[index] * images.at(j).value_mapping.slope + images.at(j).value_mapping.intercept);
            }

            if (d < min_dist)
            {
                min_dist = d;
                value_at_min = double(image_const_view.pixels[index] * images.at(j).value_mapping.slope + images.at(j).value_mapping.intercept);
            }
        }
        if (min_dist != 0)
        {
            img.pixels.ptr[i] = value_at_min;
        }
        ++i;
    }
}

// Combine multiple images into single image
api(fun with(N:1,2,3))
template<unsigned N>
// Combined image
image<N,double,shared> combine_images(
    // List of images to combine. Each image should ideally be positioned adjacent to the others.
    // The resulting image size will be the total bounding box of the images in the provided list.
    // If the provided images have overlapping pixels, the resulting pixel values for the combined image will be a sum of the overlapping image pixel values.
    // If there is a gap between the provided images or a gap between the combined bounding box and the provided images, the
    // zero value pixels will be filled in by the next closest pixel's value.
    std::vector<image<N,double,shared> > const& images)
{
    image<N,double,unique> img;

    if (images.size() <= 0) // return an image of one pixel in each direction
    {
        vector<N, unsigned> ct;
        for (unsigned i = 0; i < N; ++i)
        {
            ct[i] = unsigned(1);
        }

        // Create an image and return it
        create_image(img, ct);
        return share(img);
    }

    // Determine the limits of the images
    auto org = images.at(0).origin;
    vector<N, double> axe, end;
    for (unsigned i = 0; i < N; ++i)
    {
        axe[i] = images.at(0).axes[i][i];
        end[i] = org[i] + axe[i] * images.at(0).size[i];
    }
    for (unsigned j = 1; j < images.size(); ++j)
    {
        for (unsigned i = 0; i < N; ++i)
        {
            if (images.at(j).origin[i] < org[i])
            {
                org[i] = images.at(j).origin[i];
            }
            if (images.at(j).axes[i][i] < axe[i])
            {
                axe[i] = images.at(j).axes[i][i];
            }
            double temp_end = images.at(j).origin[i] + images.at(j).axes[i][i] * images.at(j).size[i];
            if (temp_end > end[i])
            {
                end[i] = temp_end;
            }
        }
    }

    // Compute number of pixels
    vector<N, unsigned> cts;
    for (unsigned i = 0; i < N; ++i)
    {
        double size = end[i] - org[i];
        cts[i] = unsigned(std::ceil(size / axe[i]));
    }

    // Create the image and set position/size info
    create_image(img, cts);

    img.origin = org;
    c_array<N,vector<N,double> > axes;
    for (unsigned i = 0; i < N; ++i)
    {
        for (unsigned j = 0; j < N; ++j)
        {
            axes[i][j] = 0.0;
        }
        axes[i][i] = axe[i];
    }
    img.axes = axes;

    int i = 0;
    int ii[N];
    combine_image_helper<N>(&(ii[0]), img, i, images, N);

    return share(img);
}

// Manually convert a float image to a double
api(fun with(N:1, 2, 3))
template<unsigned N>
// Float image
image<N, double, shared>
convert_float_to_double_image(
    image<N, variant, shared> const& img)
{
    image<N, double, unique> img_d;
    create_image_on_grid(img_d, get_grid(img));
    set_value_mapping(img_d, img.value_mapping.intercept, img.value_mapping.slope, img.units);

    auto image_const_view = as_const_view(cast_variant<float>(img));
    unsigned pixel_count = product(img.size);
    for (unsigned i = 0; i < pixel_count; ++i)
    {
        img_d.pixels.ptr[i] = image_const_view.pixels[i];
    }
    return share(img_d);
}

}

#endif
