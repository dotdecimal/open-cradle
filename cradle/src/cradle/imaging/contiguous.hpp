#ifndef CRADLE_IMAGING_CONTIGUOUS_HPP
#define CRADLE_IMAGING_CONTIGUOUS_HPP

#include <cradle/imaging/view_transforms.hpp>
#include <cradle/imaging/utilities.hpp>

namespace cradle {

// is_contiguous() tests if the pixels of the supplied image are arranged
// sequentially in memory, with no gaps.
template<unsigned N, class Pixel, class SP>
bool is_contiguous(image<N,Pixel,SP> const& img)
{
    size_t step = 1;
    for (unsigned i = 0; i < N; ++i)
    {
        if (img.step[i] != step)
            return false;
        step *= img.size[i];
    }
    return true;
}

// Given an image, make_contiguous() attempts to rearrange the axes of the
// supplied image such that is_contiguous will return true. If it succeeds, it
// returns the new image. If it fails, it returns none.
template<unsigned N, class Pixel, class SP>
optional<image<N,Pixel,SP> > make_contiguous(
    image<N,Pixel,SP> const& img)
{
    image<N,Pixel,SP> dst = img;
    size_t expected_step = 1;
    for (unsigned i = 0; i < N; ++i)
    {
        for (unsigned j = 0; j < N; ++j)
        {
            if (dst.step[j] == expected_step)
            {
                swap_axes(dst, i, j);
                goto found_axis;
            }
            if (-dst.step[j] == expected_step)
            {
                swap_axes(dst, i, j);
                invert_axis(dst, i);
                goto found_axis;
            }
        }
        return none;
     found_axis:
        expected_step *= img.size[i];
    }
    return dst;
}

template<unsigned N, class Pixel, class SP>
image<N,Pixel,const_view>
get_contiguous_view(image<N,Pixel,SP> const& src,
    image<N,Pixel,shared>& storage)
{
    if (is_contiguous(src))
        return cast_storage_type<const_view>(src);

    // Try to make it contiguous by rearranging the image axes.
    {
        optional<image<N,Pixel,SP> > tmp = make_contiguous(src);
        if (tmp)
            return cast_storage_type<const_view>(get(tmp));
    }

    // Otherwise create a copy of it that's contiguous.
    storage = make_eager_image_copy(src);
    return cast_storage_type<const_view>(storage);
}

template<unsigned N, class Pixel>
struct contiguous_view
{
 public:
    template<class SP>
    explicit contiguous_view(image<N,Pixel,SP> const& src)
    { view_ = get_contiguous_view(src, image_); }
    image<N,Pixel,const_view> const& get() const
    { return view_; }
 private:
    image<N,Pixel,shared> image_;
    image<N,Pixel,const_view> view_;
};

template<unsigned N>
struct contiguous_version_fn
{
    image<N,variant,shared> result;
    template<class Pixel>
    void operator()(image<N,Pixel,shared> const& src)
    {
        if (is_contiguous(src))
        {
              result = as_variant(src);
            return;
        }

        // Try to make it contiguous by rearranging the image axes.
        {
            optional<image<N,Pixel,shared> > tmp = make_contiguous(src);
            if (tmp)
            {
                result = as_variant(get(tmp));
                return;
            }
        }

        // Otherwise create a copy of it that's contiguous.
        result = as_variant(make_eager_image_copy(as_const_view(src)));
    }
};

template<unsigned N>
image<N,variant,shared>
get_contiguous_version(image<N,variant,shared> const& img)
{
    contiguous_version_fn<N> fn;
    apply_fn_to_variant(fn, img);
    return fn.result;
}

}

#endif
