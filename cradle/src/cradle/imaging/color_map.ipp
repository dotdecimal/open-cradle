#include <cradle/imaging/foreach.hpp>

namespace cradle {

namespace impl {

template<class Color>
struct apply_raw_color_map_fn
{
    std::vector<color_map_level<Color> > const* map;
    template<class Src>
    void operator()(Color& d, Src const& s)
    {
        if (s < map->front().level)
        {
            d = map->front().color;
        }
        else if (s >= map->back().level)
        {
            d = map->back().color;
        }
        else
        {
            std::size_t n_levels = map->size();
            for (std::size_t i = 1; i != n_levels; ++i)
            {
                if (s < (*map)[i].level)
                {
                    double f = (s - (*map)[i - 1].level) /
                        ((*map)[i].level - (*map)[i - 1].level);
                    d = interpolate((*map)[i - 1].color, (*map)[i].color, f);
                    break;
                }
            }
        }
    }
};

template<unsigned N, class Color, class DstSP, class SrcPixel, class SrcSP>
void apply_raw_color_map(image<N,Color,DstSP> const& dst,
    image<N,SrcPixel,SrcSP> const& src,
    std::vector<color_map_level<Color> > const& map)
{
    apply_raw_color_map_fn<Color> fn;
    fn.map = &map;
    foreach_pixel2(dst, src, fn);
}

template<unsigned N, class Color, class DstSP, class SrcPixel, class SrcSP>
void apply_color_map(image<N,Color,DstSP> const& dst,
    image<N,SrcPixel,SrcSP> const& src,
    std::vector<color_map_level<Color> > const& map)
{
    linear_function<double> inverse_value_mapping =
        inverse(src.value_mapping);
    std::size_t n_levels = map.size();
    std::vector<color_map_level<Color> > raw_map(n_levels);
    for (std::size_t i = 0; i != n_levels; ++i)
    {
        raw_map[i].level = apply(inverse_value_mapping, map[i].level);
        raw_map[i].color = map[i].color;
    }
    apply_raw_color_map(dst, src, raw_map);
}

template<unsigned N, class Color, class DstSP>
struct apply_color_map_fn
{
    image<N,Color,DstSP> const* dst;
    std::vector<color_map_level<Color> > const* map;
    template<class SrcPixel, class SrcSP>
    void operator()(image<N,SrcPixel,SrcSP> const& src)
    { apply_color_map(*dst, src, *map); }
};

template<unsigned N, class Color, class DstSP, class SrcSP>
void apply_color_map(image<N,Color,DstSP> const& dst,
    image<N,variant,SrcSP> const& src,
    std::vector<color_map_level<Color> > const& map)
{
    impl::apply_color_map_fn<N,Color,DstSP> fn;
    fn.dst = &dst;
    fn.map = &map;
    apply_fn_to_gray_variant(fn, src);
}

} // namespace impl

template<unsigned N, class Pixel, class Storage, class Color>
image<N,Color,shared>
apply_raw_color_map(
    image<N,Pixel,Storage> const& src,
    std::vector<color_map_level<Color> > const& map)
{
    image<N,Color,unique> dst;
    create_image(dst, src.size);
    copy_spatial_mapping(dst, src);
    impl::apply_raw_color_map(dst, src, map);
    return share(dst);
}

template<unsigned N, class Pixel, class Storage, class Color>
image<N,Color,shared>
apply_color_map(
    image<N,Pixel,Storage> const& src,
    std::vector<color_map_level<Color> > const& map)
{
    image<N,Color,unique> dst;
    create_image(dst, src.size);
    copy_spatial_mapping(dst, src);
    impl::apply_color_map(dst, src, map);
    return share(dst);
}

}
