#ifndef CRADLE_IMAGING_COLOR_MAP_HPP
#define CRADLE_IMAGING_COLOR_MAP_HPP

#include <cradle/imaging/variant.hpp>

namespace cradle {

api(struct with(Color:rgba8))
template<class Color>
struct color_map_level
{
    double level;
    Color color;
};

template<unsigned N, class Pixel, class Storage, class Color>
image<N,Color,shared>
apply_raw_color_map(
    image<N,Pixel,Storage> const& src,
    std::vector<color_map_level<Color> > const& map);

api(fun with(N:2,3;Pixel:variant;Storage:shared;Color:rgba8))
template<unsigned N, class Pixel, class Storage, class Color>
image<N,Color,shared>
apply_color_map(
    image<N,Pixel,Storage> const& src,
    std::vector<color_map_level<Color> > const& map);

}

#include <cradle/imaging/color_map.ipp>

#endif
