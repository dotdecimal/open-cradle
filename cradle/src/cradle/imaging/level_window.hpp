#ifndef CRADLE_IMAGING_LEVEL_WINDOW_HPP
#define CRADLE_IMAGING_LEVEL_WINDOW_HPP

#include <cradle/imaging/variant.hpp>

namespace cradle {

// Apply a level and window mapping to the given image value.
uint8_t apply_level_window(double level, double window, double image_value);

// Apply the specified level and window to the source image and return the
// resulting 8-bit image.
api(fun with(N:2,3;Pixel:variant;SP:shared))
template<unsigned N, class Pixel, class SP>
image<N,uint8_t,shared> apply_level_window(
    image<N,Pixel,SP> const& src, double level, double window);

// Same as above, but the level and window are specified as raw values.
template<unsigned N, class Pixel, class SP>
image<N,uint8_t,shared> apply_raw_level_window(
    image<N,Pixel,SP> const& src, double level, double window);

// Create a palette that will apply the given window and level to an image.
// The supplied palette must be capable of holding 2^n entries, where n is the
// number of bits in the source channel type.
template<typename SourceChannelT, typename PaletteT>
void create_level_window_palette(PaletteT* palette, double level,
    double window);

// Apply the specified window and level to the source image and place the
// result in the destination image.
// This version uses a palette to do the mapping, so it should only be used on
// 8- or 16-bit source images.
// This version works with raw integer values.
template<unsigned N, class DstT, class DstSP, class SrcT, class SrcSP>
void apply_paletted_raw_level_window(image<N,DstT,DstSP> const& dst,
    image<N,SrcT,SrcSP> const& src, double level, double window);
// This works with mapped values.
template<unsigned N, class DstT, class DstSP, class SrcT, class SrcSP>
void apply_paletted_level_window(image<N,DstT,DstSP> const& dst,
    image<N,SrcT,SrcSP> const& src, double level, double window);

}

#include <cradle/imaging/level_window.ipp>

#endif
