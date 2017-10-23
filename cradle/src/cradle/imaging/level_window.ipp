#include <cradle/imaging/apply_palette.hpp>
#include <limits>
#include <cradle/math/common.hpp>
#include <cradle/imaging/foreach.hpp>

namespace cradle {

struct level_window_pixel_fn
{
    double slope, intercept;
    template<class Src>
    void operator()(uint8_t& dst, Src const& src)
    { dst = uint8_t(clamp((src - intercept) * slope, 0., 255.)); }
};

template<unsigned N, class Pixel, class SP>
image<N,uint8_t,shared> apply_raw_level_window(
    image<N,Pixel,SP> const& src, double level, double window)
{
    image<N,uint8_t,unique> dst;
    create_image(dst, src.size);
    copy_spatial_mapping(dst, src);

    level_window_pixel_fn fn;
    fn.slope = 255. / window;
    fn.intercept = level - window / 2;
    foreach_pixel2(dst, src, fn);

    return share(dst);
}

template<unsigned N, class Pixel, class SP>
image<N,uint8_t,shared> apply_level_window(
    image<N,Pixel,SP> const& src, double level, double window)
{
    linear_function<double> inverse_value_mapping =
        inverse(src.value_mapping);
    return apply_raw_level_window(src,
        apply(inverse_value_mapping, level),
        apply(inverse_value_mapping, window) -
        apply(inverse_value_mapping, 0));
}

namespace impl {
    template<unsigned N>
    struct apply_level_window_fn
    {
        image<N,uint8_t,shared> result;
        double level, window;
        template<class Pixel, class SP>
        void operator()(image<N,Pixel,SP> const& src)
        { result = apply_level_window(src, level, window); }
    };
}

template<unsigned N, class SP>
image<N,uint8_t,shared> apply_level_window(
    image<N,variant,SP> const& src, double level, double window)
{
    impl::apply_level_window_fn<N> fn;
    fn.level = level;
    fn.window = window;
    apply_fn_to_gray_variant(fn, src);
    return fn.result;
}

template<typename SourceChannelT, typename PaletteT>
void create_level_window_palette(PaletteT* palette, double level,
    double window)
{
    typedef SourceChannelT source_t;
    typedef typename unsigned_channel_type<source_t>::type unsigned_t;

    int const max_i = int((std::numeric_limits<source_t>::max)());
    int const min_i = int((std::numeric_limits<source_t>::min)());

    // Fill the part before the ramp with all zeros.
    double real_top = level - window / 2;
    int const top = (std::min)(int(real_top + 0.5), max_i + 1);
    int i = min_i;
    for (; i < top; ++i)
    {
        unsigned_t index = unsigned_t(SourceChannelT(i));
        palette[index] = PaletteT(0);
    }

    double const inc = double((std::numeric_limits<PaletteT>::max)()) / window;
    double n = inc * (i - real_top);

    // Fill in the ramp.
    int const bottom = (std::min)(top + int(window + 0.5), max_i + 1);
    for (; i < bottom; i++)
    {
        unsigned_t index = unsigned_t(SourceChannelT(i));
        palette[index] = PaletteT(n);
        n += inc;
    }

    // Fill the part after the ramp with the maximum intensity.
    PaletteT max = (std::numeric_limits<PaletteT>::max)();
    for (; i <= max_i; ++i)
    {
        unsigned_t index = unsigned_t(SourceChannelT(i));
        palette[index] = PaletteT(max);
    }
}

template<unsigned N, class DstT, class DstSP, class SrcT, class SrcSP>
void apply_paletted_raw_level_window(image<N,DstT,DstSP> const& dst,
    image<N,SrcT,SrcSP> const& src, double level, double window)
{
    // Create the appropriate palette.
    std::vector<DstT> palette(int((std::numeric_limits<
        typename unsigned_channel_type<SrcT>::type>::max)()) + 1);
    create_level_window_palette<SrcT>(&palette[0], level, window);
    // Apply it to create the output image.
    apply_palette(dst, src, &palette[0]);
}

template<unsigned N, class DstT, class DstSP, class SrcT, class SrcSP>
void apply_paletted_level_window(image<N,DstT,DstSP> const& dst,
    image<N,SrcT,SrcSP> const& src, double level, double window)
{
    linear_function<double> inverse_value_mapping =
        inverse(src.value_mapping);
    apply_paletted_raw_level_window(dst, src,
        apply(inverse_value_mapping, level),
        (apply(inverse_value_mapping, window) -
            apply(inverse_value_mapping, 0)));
}

}
