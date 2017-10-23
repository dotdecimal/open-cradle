#ifndef CRADLE_IMAGING_IMAGE_HPP
#define CRADLE_IMAGING_IMAGE_HPP

#include <cradle/geometry/common.hpp>
#include <cradle/units.hpp>
#include <cradle/imaging/forward.hpp>
#include <cradle/imaging/color.hpp>

namespace cradle {

// An image is a N-dimensional array of pixels that lie on a regular grid.
// In addition to raw pixel data, the image structure provides information
// about the position and orientation of the image within space and the
// relationship of pixel values to real values.
//
// The structure is templated such that pixels can be of any type and can be
// stored or generated in any way. The latter is controlled by the Storage
// parameter, which is a policy structure that must provide the following
// metafunctions...
//
// Storage::pointer_type<N,Pixel>::type yields the type used by the image
// structure to reference pixels.
//
// Storage::iterator_type<N,Pixel>::type yields the type used to reference
// pixels during iteration. A value of this type can be created by calling
// get_iterator(ptr) on a value of the above pointer type. Additionally, it
// must be possible to dereference a value of this type to yield a pixel.
//
// Storage::reference_type<N,Pixel>::type yields the type used to reference
// pixels during iteration. This is generally either Pixel&, Pixel const&,
// or simply Pixel.
//
// Storage::step_type<N,Pixel>::type yields the type used to store the step
// from one pixel to another. It must be possible to add and subtract values
// of this type and to multiply them by a scalar. In addition, it must be
// possible to add (or subtract) a step value to (or from) a pointer or
// iterator to yield another pointer or iterator, respectively. All of these
// operations are done using the normal arithmetic operators.

// untyped_image_base provides all the information about an image that's not
// dependent on the underlying pixel and storage types.
// Occasionally, you can write functions that operate directly on this type
// and avoid lots of unnecessary templating.
template<unsigned N>
struct untyped_image_base
{
    // number of pixels in the image
    vector<N,unsigned> size;

    // mapping from image space to real space
    // origin is the location of the outside corner of the first pixel.
    // axes are vectors representing the image axes in real space.
    // The length of each axis vector is one pixel.
    vector<N,double> origin;
    c_array<N,vector<N,double> > axes;

    // mapping from pixel channel values to real values
    linear_function<double> value_mapping;
    // the units of the real values
    cradle::units units;
};

template<unsigned N, class Pixel, class Storage>
struct image : untyped_image_base<N>
{
    // allow external access to template parameters
    static unsigned const dimensionality = N;
    typedef Pixel pixel_type;
    typedef Storage storage;

    // types assigned by the storage policy
    typedef typename Storage::template pointer_type<N,Pixel>::type
        pointer_type;
    typedef typename Storage::template iterator_type<N,Pixel>::type
        iterator_type;
    typedef typename Storage::template reference_type<N,Pixel>::type
        reference_type;
    typedef typename Storage::template step_type<N,Pixel>::type step_type;

    // pointer to the first pixel
    pointer_type pixels;

    // an N-dimensional vector giving the step size along each axis
    // step[i] is the step from one pixel to the next in dimension i
    vector<N,step_type> step;

    // default constructor
    image()
    {
        this->size = uniform_vector<N,unsigned>(0);
        this->origin = uniform_vector<N,double>(0);
        for (unsigned i = 0; i != N; ++i)
        {
            for (unsigned j = 0; j != N; ++j)
                this->axes[i][j] = (i == j) ? 1 : 0;
        }
        this->value_mapping = linear_function<double>(0, 1);
        this->units = no_units;
    }
    // constructor with all fields (value mapping optional)
    image(pointer_type const& pixels,
        vector<N,unsigned> const& size,
        vector<N,step_type> const& step,
        vector<N,double> const& origin,
        c_array<N,vector<N,double> > const& axes,
        linear_function<double> const& value_mapping =
            linear_function<double>(0, 1),
        cradle::units const& units = no_units)
    {
        this->pixels = pixels;
        this->size = size;
        this->step = step;
        this->origin = origin;
        this->axes = axes;
        this->value_mapping = value_mapping;
        this->units = units;
    }
    // constructor with all fields except spatial mapping
    // (value mapping optional)
    image(pointer_type const& pixels,
        vector<N,unsigned> const& size,
        vector<N,step_type> const& step,
        linear_function<double> const& value_mapping =
            linear_function<double>(0, 1),
        cradle::units const& units = no_units)
    {
        this->pixels = pixels;
        this->size = size;
        this->step = step;
        this->origin = uniform_vector<N,double>(0);
        for (unsigned i = 0; i != N; ++i)
        {
            for (unsigned j = 0; j != N; ++j)
                this->axes[i][j] = (i == j) ? 1 : 0;
        }
        this->value_mapping = value_mapping;
        this->units = units;
    }
};

template<unsigned N, class Pixel, class Storage>
void swap(image<N,Pixel,Storage>& a, image<N,Pixel,Storage>& b)
{
    using std::swap;
    swap(a.pixels, b.pixels);
    swap(a.size, b.size);
    swap(a.step, b.step);
    swap(a.origin, b.origin);
    for (unsigned i = 0; i != N; ++i)
        swap(a.axes[i], b.axes[i]);
    swap(a.value_mapping, b.value_mapping);
    swap(a.units, b.units);
}

// UTILITY FUNCTIONS - Various utility functions for working with images
// structures.

template<unsigned N>
void copy_untyped_image_info(untyped_image_base<N>& dst,
    untyped_image_base<N> const& src)
{
    dst.size = src.size;
    dst.origin = src.origin;
    for (unsigned i = 0; i != N; ++i)
        dst.axes[i] = src.axes[i];
    dst.value_mapping = src.value_mapping;
    dst.units = src.units;
}

template<unsigned N>
bool empty(untyped_image_base<N> const& img)
{
    return product(img.size) == 0;
}

// Calculate the step values for an image with contiguous pixels.
template<unsigned N>
vector<N,ptrdiff_t> get_contiguous_steps(
    vector<N,unsigned> const& size)
{
    vector<N,ptrdiff_t> r;
    ptrdiff_t step = 1;
    for (unsigned i = 0; i < N; ++i)
    {
        r[i] = step;
        step *= size[i];
    }
    return r;
}

// Swap two axes on an image.
template<unsigned N, class Pixel, class Storage>
void swap_axes(image<N,Pixel,Storage>& img, unsigned a, unsigned b)
{
    using std::swap;
    if (a == b)
        return;
    swap(img.size[a], img.size[b]);
    swap(img.step[a], img.step[b]);
    swap(img.axes[a], img.axes[b]);
}

// Determine if two images refer to the same pixel data.
template<unsigned N, class Pixel, class Storage>
bool same_pixel_data(image<N,Pixel,Storage> const& a,
    image<N,Pixel,Storage> const& b)
{
    return a.pixels == b.pixels && a.size == b.size && a.step == b.step;
}

template<unsigned N>
void reset_spatial_mapping(untyped_image_base<N>& img)
{
    img.origin = uniform_vector<N,double>(0);
    for (unsigned i = 0; i != N; ++i)
    {
        for (unsigned j = 0; j != N; ++j)
            img.axes[i][j] = (i == j) ? 1 : 0;
    }
}

template<unsigned N>
void set_value_mapping(untyped_image_base<N>& img, double intercept,
    double slope, units const& units)
{
    img.value_mapping = linear_function<double>(intercept, slope);
    img.units = units;
}

template<unsigned N>
void reset_value_mapping(untyped_image_base<N>& img)
{
    set_value_mapping(img, 0, 1, no_units);
}

template<unsigned N>
void copy_spatial_mapping(untyped_image_base<N>& dst,
    untyped_image_base<N> const& src)
{
    dst.origin = src.origin;
    for (unsigned i = 0; i < N; ++i)
        dst.axes[i] = src.axes[i];
}

template<unsigned N1, unsigned N2>
void copy_value_mapping(untyped_image_base<N1>& dst,
    untyped_image_base<N2> const& src)
{
    dst.value_mapping = src.value_mapping;
    dst.units = src.units;
}

template<unsigned N>
bool same_spatial_mapping(untyped_image_base<N> const& a,
    untyped_image_base<N> const& b)
{
    for (unsigned i = 0; i != N; ++i)
    {
        if (a.axes[i] != b.axes[i])
            return false;
    }
    return a.origin == b.origin;
}

template<unsigned N1, unsigned N2>
bool same_value_mapping(untyped_image_base<N1> const& a,
    untyped_image_base<N2> const& b)
{
    return a.value_mapping == b.value_mapping && a.units == b.units;
}

template<class Step>
inline void cast_step(Step& dst, Step const& src)
{ dst = src; }

template<class Pointer>
inline void cast_pointer(Pointer& dst, Pointer const& src)
{ dst = src; }

// Cast an image from one type to another (only the dimensionality must match).
template<class DstImage, unsigned N, class SrcPixel, class SrcStorage>
DstImage cast_image(image<N,SrcPixel,SrcStorage> const& img)
{
    DstImage r;
    cast_pointer(r.pixels, img.pixels);
    r.size = img.size;
    cast_step(r.step, img.step);
    copy_spatial_mapping(r, img);
    copy_value_mapping(r, img);
    return r;
}

// Cast an image from one storage type to another.
template<class ToStorage, unsigned N, class Pixel, class FromStorage>
image<N,Pixel,ToStorage> cast_storage_type(
    image<N,Pixel,FromStorage> const& img)
{
    return cast_image<image<N,Pixel,ToStorage> >(img);
}

// Are the image contents lazily computed?
// This can be specialized for images with different types.
template<unsigned N, class Pixel, class Storage>
bool is_lazy(image<N,Pixel,Storage> const& img)
{ return false; }

// PIXEL UTILITIES - Conceptually, a pixel can have multiple channels
// (e.g., R, G, and B). To work with pixels generically, your code should use
// the following functions to manipulate them. These can be overridden for
// more complex pixel types.

template<class Pixel>
double apply_linear_function(linear_function<double> const& f, Pixel x)
{ return apply(f, x); }

template<class Pixel, class Value>
void fill_channels(Pixel& p, Value v)
{ p = v; }

// STORAGE POLICIES - The following are storage policies that can be used with
// the image structure...

// The view policy references image pixels using normal C pointers. It provides
// no ownership of the underlying pixel data. Thus, what you are storing is a
// view of an image rather than actual ownership of the image.

template<unsigned N, class Pixel>
struct view_pointer_type
{
    typedef Pixel* type;
};
template<unsigned N, class Pixel>
struct view_iterator_type
{
    typedef Pixel* type;
};
template<unsigned N, class Pixel>
struct view_reference_type
{
    typedef Pixel& type;
};
template<unsigned N, class Pixel>
struct view_step_type
{
    typedef ptrdiff_t type;
};
struct view
{
    template<unsigned N, class Pixel>
    struct pointer_type : view_pointer_type<N,Pixel> {};
    template<unsigned N, class Pixel>
    struct iterator_type : view_iterator_type<N,Pixel> {};
    template<unsigned N, class Pixel>
    struct reference_type : view_reference_type<N,Pixel> {};
    template<unsigned N, class Pixel>
    struct step_type : view_step_type<N,Pixel> {};
};
template<class Pixel>
inline Pixel* get_iterator(Pixel* ptr)
{ return ptr; }

// Create a view of some contiguous pixel data.
template<unsigned N, class Pixel>
image<N,Pixel,view> make_view(
    Pixel* pixels, vector<N,unsigned> const& size)
{
    return image<N,Pixel,view>(pixels, size, get_contiguous_steps(size));
}

template<unsigned N, class Pixel, class SP>
image<N,Pixel,view> as_view(image<N,Pixel,SP> const& img)
{ return cast_storage_type<view>(img); }

// The const_view policy is the same as the view policy, but it only allows
// const access to the underlying pixels. To keep things consistent, you should
// use image<N,T,const_view> rather than image<N,T const,view>.

// Note that const_view is declared in forward.hpp.

template<class Pixel>
inline Pixel const* get_iterator(Pixel const* ptr)
{ return ptr; }
template<class Pixel>
inline void cast_pointer(Pixel const*& dst, Pixel* src)
{ dst = src; }

// Create a const view of some external, contiguous pixel data.
template<unsigned N, class Pixel>
image<N,Pixel,const_view> make_const_view(
    Pixel const* pixels, vector<N,unsigned> const& size)
{
    return image<N,Pixel,const_view>(pixels, size, get_contiguous_steps(size));
}

// Create a const view of some external, contiguous pixel data.
template<unsigned N, class Pixel>
image<N, Pixel, const_view>
make_const_view(
    Pixel const* pixels,
    vector<N, unsigned> const& size,
    vector<N, ptrdiff_t> const& steps)
{
    return image<N, Pixel, const_view>(pixels, size, steps);
}

template<unsigned N, class Pixel, class SP>
image<N,Pixel,const_view> as_const_view(image<N,Pixel,SP> const& img)
{ return cast_storage_type<const_view>(img); }

// The shared policy stores a shared_ptr to the image pixels, so it supports
// shared ownership of the underlying pixel data. It only allows const access
// to the underlying pixel data.

// Note that many image operations require adjusting the pixel pointer in the
// image structure (e.g., to flip the image, create a view of a subregion,
// etc.) If we want to be able to do this while still maintaining ownership,
// we actually need two pointers: one for ownership and one for the view.

// Note that shared_pointer and the shared policy are declared in forward.hpp.

template<class Pixel>
inline void swap(shared_pointer<Pixel>& a, shared_pointer<Pixel>& b)
{
    using std::swap;
    swap(a.ownership, b.ownership);
    swap(a.view, b.view);
}
template<class Pixel>
inline bool operator==(
    shared_pointer<Pixel> const& a, shared_pointer<Pixel> const& b)
{
    return a.view == b.view;
}
template<class Pixel>
inline bool operator!=(
    shared_pointer<Pixel> const& a, shared_pointer<Pixel> const& b)
{
    return !(a == b);
}

template<class Pixel>
inline shared_pointer<Pixel> operator+(
    shared_pointer<Pixel> const& ptr, ptrdiff_t step)
{
    shared_pointer<Pixel> r;
    r.ownership = ptr.ownership;
    r.view = ptr.view + step;
    return r;
}
template<class Pixel>
inline shared_pointer<Pixel>& operator+=(
    shared_pointer<Pixel>& ptr, ptrdiff_t step)
{
    ptr.view += step;
    return ptr;
}
template<class Pixel>
inline shared_pointer<Pixel> operator-(
    shared_pointer<Pixel> const& ptr, ptrdiff_t step)
{
    shared_pointer<Pixel> r;
    r.ownership = ptr.ownership;
    r.view = ptr.view - step;
    return r;
}
template<class Pixel>
inline shared_pointer<Pixel>& operator-=(
    shared_pointer<Pixel>& ptr, ptrdiff_t step)
{
    ptr.view -= step;
    return ptr;
}

template<class Pixel>
inline Pixel const* get_iterator(shared_pointer<Pixel> const& ptr)
{ return ptr.view; }

template<class Pixel>
inline void cast_pointer(Pixel const*& dst, shared_pointer<Pixel> const& src)
{ dst = src.view; }
template<class Pixel>
inline void cast_pointer(shared_pointer<Pixel>& dst, Pixel const* src)
{
    dst.ownership.reset();
    dst.view = src;
}

// An array_deleter that allows releasing ownership of its contents.
template <typename T>
struct releasable_array_deleter
{
    releasable_array_deleter() : released_(false) {}
    void release() { released_ = true; }
    void operator()(T* ptr) { if (!released_) delete[] ptr; }
 private:
    bool released_;
};

// Initialized a shared_pointer with the given pixel data.
// (This assumes ownership of the pixel data.)
template<class Pixel>
void initialize_shared_pixel_pointer(shared_pointer<Pixel>& ptr, Pixel* pixels)
{
    ptr.ownership =
        alia__shared_ptr<Pixel>(pixels, releasable_array_deleter<Pixel>());
    ptr.view = pixels;
}

// Determine if a shared_pointer has sole ownership of its pixel data.
template<class Pixel>
bool has_sole_ownership(shared_pointer<Pixel> const& ptr)
{
    auto* owner = any_cast<alia__shared_ptr<Pixel> >(ptr.ownership);
    return owner && owner->unique();
}

// Release ownership of the given shared pixel data.
// The supplied pointer must have unique ownership of the pixel data.
// The return value is the pixel data.
// (The caller must assume ownership of it.)
template<class Pixel>
Pixel* release_ownership(shared_pointer<Pixel>& ptr)
{
    assert(has_sole_ownership(ptr));
    auto* owner = const_cast<alia__shared_ptr<Pixel>*>(
        any_cast<alia__shared_ptr<Pixel> >(ptr.ownership));
    releasable_array_deleter<Pixel>* deleter =
        std::get_deleter<releasable_array_deleter<Pixel> >(*owner);
    assert(deleter);
    deleter->release();
    Pixel* pixels = owner->get();
    owner->reset();
    ptr.view = 0;
    return pixels;
}

// The unique policy also uses a C pointer but provides unique ownership of
// the underlying pixel data. Images that use this policy are not copyable.
// Instead, you must cast them to image<N,Pixel,view> to manipulate them.

// The unique policy is useful for initially creating images since it allows
// mutation. Once it's created, if you want to share it, you can call
// share() to consume the unique image and create a shared one.

template<class Pixel>
struct unique_pointer : noncopyable
{
    unique_pointer() : ptr(0) {}
    ~unique_pointer() { delete[] ptr; }
    Pixel* ptr;
};

template<class Pixel>
inline void swap(unique_pointer<Pixel>& a, unique_pointer<Pixel>& b)
{
    using std::swap;
    swap(a.ptr, b.ptr);
};

template<class Pixel>
inline bool operator==(
    unique_pointer<Pixel> const& a, unique_pointer<Pixel> const& b)
{
    return a.ptr == b.ptr;
}
template<class Pixel>
inline bool operator!=(
    unique_pointer<Pixel> const& a, unique_pointer<Pixel> const& b)
{
    return !(a == b);
}

template<class Pixel>
inline Pixel* get_iterator(unique_pointer<Pixel> const& ptr)
{ return ptr.ptr; }

template<class Pixel>
inline void cast_pointer(Pixel*& dst, unique_pointer<Pixel> const& src)
{ dst = src.ptr; }
template<class Pixel>
inline void cast_pointer(Pixel const*& dst, unique_pointer<Pixel> const& src)
{ dst = src.ptr; }

template<unsigned N, class Pixel>
struct unique_pointer_type
{
    typedef unique_pointer<Pixel> type;
};
template<unsigned N, class Pixel>
struct unique_reference_type
{
    typedef Pixel& type;
};
template<unsigned N, class Pixel>
struct unique_iterator_type
{
    typedef Pixel* type;
};

template<unsigned N, class Pixel>
struct unique_step_type
{
    typedef ptrdiff_t type;
};
struct unique
{
    template<unsigned N, class Pixel>
    struct pointer_type : unique_pointer_type<N,Pixel> {};
    template<unsigned N, class Pixel>
    struct iterator_type : unique_iterator_type<N,Pixel> {};
    template<unsigned N, class Pixel>
    struct reference_type : unique_reference_type<N,Pixel> {};
    template<unsigned N, class Pixel>
    struct step_type : unique_step_type<N,Pixel> {};
};

// Given a unique image, this consumes the image and creates a shared image
// with the same contents.
// (It uses the same pixel pointer, so no pixel copying takes place.)
template<unsigned N, class Pixel>
image<N,Pixel,shared>
share(image<N,Pixel,unique>& img)
{
    shared_pointer<Pixel> ptr;
    initialize_shared_pixel_pointer(ptr, img.pixels.ptr);
    img.pixels.ptr = 0;
    return image<N,Pixel,shared>(ptr, img.size, img.step, img.origin, img.axes,
        img.value_mapping, img.units);
}

// Create a unique image from existing pixel data.
// The image assumes ownership of the pixel data.
template<unsigned N, class Pixel>
void create_image(image<N,Pixel,unique>& img, vector<N,unsigned> const& size,
    Pixel* pixels)
{
    delete[] img.pixels.ptr;
    img.pixels.ptr = pixels;
    img.size = size;
    img.step = get_contiguous_steps(size);
    reset_spatial_mapping(img);
    reset_value_mapping(img);
}

// Create a new unique image with the given size.
template<unsigned N, class Pixel>
void create_image(image<N,Pixel,unique>& img, vector<N,unsigned> const& size)
{
    create_image(img, size, new Pixel[product(vector<N,size_t>(size))]);
}

// Creates an empty shared image.
template<unsigned N, class Pixel>
image<N,Pixel,shared> empty_image()
{
    image<N,Pixel,unique> img;
    img.pixels.ptr = 0;
    img.size = uniform_vector<N,unsigned>(0);
    img.step = get_contiguous_steps(img.size);
    reset_spatial_mapping(img);
    reset_value_mapping(img);
    return share(img);
}

// Get an iterator that refers to a particular pixel within an image.
template<unsigned N, class Pixel, class Storage>
typename Storage::template iterator_type<N,Pixel>::type
get_pixel_iterator(
    image<N,Pixel,Storage> const& img, vector<N,unsigned> const& index)
{
    typename Storage::template iterator_type<N,Pixel>::type p =
        get_iterator(img.pixels);
    for (unsigned i = 0; i < N; ++i)
        p += index[i] * img.step[i];
    return p;
}

// It's sometimes useful to get an iterator to a pixel that's not guaranteed
// to be within the image, so we provide a version that takes a signed index.
template<unsigned N, class Pixel, class Storage>
typename Storage::template iterator_type<N,Pixel>::type
get_pixel_iterator(
    image<N,Pixel,Storage> const& img, vector<N,int> const& index)
{
    typename Storage::template iterator_type<N,Pixel>::type p =
        get_iterator(img.pixels);
    for (unsigned i = 0; i < N; ++i)
        p += index[i] * img.step[i];
    return p;
}

// Get a reference to a particular pixel within an image.
template<unsigned N, class Pixel, class Storage>
typename Storage::template reference_type<N,Pixel>::type
get_pixel_ref(
    image<N,Pixel,Storage> const& img, vector<N,unsigned> const& index)
{
    return *get_pixel_iterator(img, index);
}

// If an image type meets the following requirements, it's considered a regular
// CRADLE type:
// * dimensionality is 1, 2, or 3
// * Pixel is a type that's compatible with variant images
// * StoragePolicy is shared

#define CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE(N, T) \
    bool operator==(image<N,T,shared> const& a, image<N,T,shared> const& b); \
    bool operator!=(image<N,T,shared> const& a, image<N,T,shared> const& b); \
    bool operator<(image<N,T,shared> const& a, image<N,T,shared> const& b); \
    void to_value(value* v, image<N,T,shared> const& x); \
    void from_value(image<N,T,shared>* x, value const& v); \
    void read_fields_from_immutable_map(image<N,T,shared>& x, \
        std::map<string,untyped_immutable> const& fields); \
    size_t deep_sizeof(image<N,T,shared> const& x); \
    std::ostream& operator<<(std::ostream& s, image<N,T,shared> const& x); \
    static inline raw_type_info get_type_info(image<N,T,shared> const& x) \
    { \
        return raw_type_info(raw_kind::NAMED_TYPE_REFERENCE, \
            any(raw_named_type_reference(string("dosimetry"), \
                string("image_" #N "d")))); \
    } \
    raw_type_info get_proper_type_info(image<N,T,shared> const& x); \
    } namespace std { \
    template<> \
    struct hash<cradle::image<N,T,cradle::shared> > \
    { \
        size_t operator()(cradle::image<N,T,cradle::shared> const& x) const; \
    }; \
    } namespace cradle {

#define CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(T) \
    CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE(1,T) \
    CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE(2,T) \
    CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE(3,T)

CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::int8_t)
CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::uint8_t)
CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::int16_t)
CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::uint16_t)
CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::int32_t)
CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::uint32_t)
CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::int64_t)
CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::uint64_t)
CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(float)
CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(double)

CRADLE_DECLARE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::rgba8)

// Register the regular image types with the given API.
struct api_implementation;
void register_image_types(api_implementation& api);

}

#endif
