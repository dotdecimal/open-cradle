#ifndef CRADLE_IMAGING_HISTOGRAM_EQUALIZE_HPP
#define CRADLE_IMAGING_HISTOGRAM_EQUALIZE_HPP

#include <cradle/imaging/histogram.hpp>
#include <cradle/imaging/apply_palette.hpp>
#include <cradle/imaging/view_transforms.hpp>
#include <vector>
#include <limits>
#include <boost/type_traits/remove_cv.hpp>

namespace cradle {

// Histogram equalize the source image and place the result in the destination
// image. Only use this on 8- or 16-bit images.
template<unsigned N, class DstT, class DstSP, class SrcT, class SrcSP>
void histogram_equalize(image<N,DstT,DstSP> const& dst,
    image<N,SrcT,SrcSP> const& src)
{
    typedef typename unsigned_channel_type<SrcT>::type unsigned_t;
    std::vector<DstT> palette(int(std::numeric_limits<unsigned_t>::max()) + 1);
    create_histogram_equalization_palette(src, &palette[0]);
    apply_palette(dst, src, &palette[0]);
}

// Fill in the given palette such that it will histogram equalize the given
// image if applied to it.
// The supplied palette should be capable of holding 2^n entries, where n is
// the number of bits in the source image's channel type.
template<unsigned N, class SrcT, class SrcSP, class PaletteT>
void create_histogram_equalization_palette(image<N,SrcT,SrcSP> const& img,
    PaletteT* palette)
{
    // Compute a histogram of the image.  Using a subsampled view speeds up
    // the process significantly, and unless the image contains a very strong,
    // regular, high-frequency pattern, it shouldn't affect the result much.
    std::vector<unsigned> hist;
    if (product(img.size) > 10000)
    {
        compute_raw_histogram(&hist,
            subsampled_view(img, make_vector<unsigned>(4, 4)));
    }
    else
        compute_raw_histogram(&hist, img);

    typedef typename unsigned_channel_type<SrcT>::type unsigned_t;
    typedef typename boost::remove_cv<SrcT>::type source_t;

    int const min_channel_v = int(std::numeric_limits<source_t>::min());
    int const max_channel_v = int(std::numeric_limits<source_t>::max());

    // Do a cumulative integral of the histogram.
    int sum = 0, highest = 0;
    for (int i = min_channel_v; i <= max_channel_v; ++i)
    {
        unsigned_t index = unsigned_t(SrcT(i));
        int v = hist[index];
        if (v != 0)
            highest = sum;
        hist[index] = sum;
        sum += v;
    }

    // Create the palette by scaling the integrated histogram to fill all
    // possible values in the palette's channel type.
    float scale = float(std::numeric_limits<PaletteT>::max()) / highest;
    for (int i = min_channel_v; i <= max_channel_v; ++i)
    {
        unsigned_t index = unsigned_t(SrcT(i));
        int tmp = int(scale * hist[index]);
        palette[index] = PaletteT(std::min(tmp,
            int(std::numeric_limits<PaletteT>::max())));
    }
}

}

#endif
