#ifndef CRADLE_IMAGING_APPLY_PALETTE_HPP
#define CRADLE_IMAGING_APPLY_PALETTE_HPP

#include <cradle/imaging/channel.hpp>
#include <cradle/imaging/foreach.hpp>

namespace cradle {

template<class Palette>
struct apply_palette_fn
{
    Palette const* palette;
    template<class Dst, class Src>
    void operator()(Dst& dst, Src const& src)
    { dst = palette[typename unsigned_channel_type<Src>::type(src)]; }
};

// Given a grayscale source image, this applies the given palette to fill the
// destination image.
template<unsigned N, class DstPixel, class DstStorage,
    class SrcPixel, class SrcStorage, class Palette>
void apply_palette(image<N,DstPixel,DstStorage> const& dst,
    image<N,SrcPixel,SrcStorage> const& src, Palette const* palette)
{
    apply_palette_fn<Palette> fn;
    fn.palette = palette;
    foreach_pixel2(dst, src, fn);
}

}

#endif
