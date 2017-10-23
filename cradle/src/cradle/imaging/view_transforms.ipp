namespace cradle {

namespace impl {
    template<unsigned N, class SP>
    struct aligned_view_fn
    {
        image<N,variant,SP> dst;
        template<class Pixel>
        void operator()(image<N,Pixel,SP> const& img)
        { dst = as_variant(aligned_view(img)); }
    };
}
template<unsigned N, class SP>
image<N,variant,SP> aligned_view(image<N,variant,SP> const& img)
{
    impl::aligned_view_fn<N,SP> fn;
    apply_fn_to_variant(fn, img);
    return fn.dst;
}

namespace impl {
    template<unsigned N, class SP>
    struct subimage_view_fn
    {
        image<N,variant,SP> dst;
        box<N,unsigned> region;
        template<class Pixel>
        void operator()(image<N,Pixel,SP> const& img)
        { dst = as_variant(subsampled_view(img, region)); }
    };
}
template<unsigned N, class SP>
image<N,variant,SP> subimage_view(image<N,variant,SP> const& img,
    box<N,unsigned> const& region)
{
    impl::subimage_view_fn<N,SP> fn;
    fn.region = region;
    apply_fn_to_variant(fn, img);
    return fn.dst;
}

namespace impl {
    template<unsigned N, class SP>
    struct subsampled_view_fn
    {
        image<N,variant,SP> dst;
        vector<N,unsigned> step;
        template<class Pixel>
        void operator()(image<N,Pixel,SP> const& img)
        { dst = as_variant(subsampled_view(img, step)); }
    };
}
template<unsigned N, class SP>
image<N,variant,SP> subsampled_view(image<N,variant,SP> const& img,
    vector<N,unsigned> const& step)
{
    impl::subsampled_view_fn<N,SP> fn;
    fn.step = step;
    apply_fn_to_variant(fn, img);
    return fn.dst;
}

namespace impl {
    template<unsigned N, class SP>
    struct raw_flipped_view_fn
    {
        image<N,variant,SP> dst;
        unsigned axis;
        template<class Pixel>
        void operator()(image<N,Pixel,SP> const& img)
        { dst = as_variant(raw_flipped_view(img, axis)); }
    };
}
template<unsigned N, class SP>
image<N,variant,SP> raw_flipped_view(image<N,variant,SP> const& img, unsigned axis)
{
    impl::raw_flipped_view_fn<N,SP> fn;
    fn.axis = axis;
    apply_fn_to_variant(fn, img);
    return fn.dst;
}

namespace impl {
    template<unsigned N, class SP>
    struct invert_axis_fn
    {
        image<N,variant,SP> dst;
        unsigned axis;
        template<class Pixel>
        void operator()(image<N,Pixel,SP> const& img)
        {
            image<N,Pixel,SP> tmp = img;
            invert_axis(tmp, axis);
            dst = as_variant(tmp);
        }
    };
}
template<unsigned N, class SP>
void invert_axis(image<N,variant,SP>& img, unsigned axis)
{
    impl::invert_axis_fn<N,SP> fn;
    fn.axis = axis;
    apply_fn_to_variant(fn, img);
    img = fn.dst;
}

namespace impl {
    template<class SP>
    struct raw_rotated_180_view_fn
    {
        image<2,variant,SP> dst;
        template<class Pixel>
        void operator()(image<2,Pixel,SP> const& img)
        {
            image<2,Pixel,SP> tmp = img;
            raw_rotated_180_view(tmp);
            dst = as_variant(tmp);
        }
    };
}
template<class SP>
void raw_rotated_180_view(image<2,variant,SP>& img)
{
    impl::raw_rotated_180_view_fn<SP> fn;
    apply_fn_to_variant(fn, img);
    img = fn.dst;
}

namespace impl {
    template<class SP>
    struct raw_rotated_90ccw_view_fn
    {
        image<2,variant,SP> dst;
        template<class Pixel>
        void operator()(image<2,Pixel,SP> const& img)
        {
            image<2,Pixel,SP> tmp = img;
            raw_rotated_90ccw_view(tmp);
            dst = as_variant(tmp);
        }
    };
}
template<class SP>
void raw_rotated_90ccw_view(image<2,variant,SP>& img)
{
    impl::raw_rotated_90ccw_view_fn<SP> fn;
    apply_fn_to_variant(fn, img);
    img = fn.dst;
}

namespace impl {
    template<class SP>
    struct raw_rotated_90cw_view_fn
    {
        image<2,variant,SP> dst;
        template<class Pixel>
        void operator()(image<2,Pixel,SP> const& img)
        {
            image<2,Pixel,SP> tmp = img;
            raw_rotated_90cw_view(tmp);
            dst = as_variant(tmp);
        }
    };
}
template<class SP>
void raw_rotated_90cw_view(image<2,variant,SP>& img)
{
    impl::raw_rotated_90cw_view_fn<SP> fn;
    apply_fn_to_variant(fn, img);
    img = fn.dst;
}

}
