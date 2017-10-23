#ifndef CRADLE_IMAGING_GEOMETRY_HPP
#define CRADLE_IMAGING_GEOMETRY_HPP

#include <cradle/geometry/common.hpp>
#include <cradle/geometry/transformations.hpp>
#include <cradle/imaging/image.hpp>
#include <cradle/geometry/regular_grid.hpp>

namespace cradle {

template<unsigned N>
matrix<N+1,N+1,double>
get_spatial_mapping(untyped_image_base<N> const& img)
{
    matrix<N+1,N+1,double> rotation;
    for (unsigned i = 0; i < N; ++i)
        rotation.set_column(i, unslice(img.axes[i], N, 0));
    rotation.set_column(N,
        unslice(uniform_vector<N,double>(0), N, 1));
    return translation(vector<N,double>(img.origin)) *
        rotation;
}

template<unsigned N>
void set_spatial_mapping(untyped_image_base<N>& img,
    vector<N,double> const& origin,
    vector<N,double> const& spacing)
{
    img.origin = origin;
    for (unsigned i = 0; i < N; ++i)
    {
        for (unsigned j = 0; j != N; ++j)
            img.axes[i][j] = (i == j) ? spacing[i] : 0;
    }
}

template<unsigned N>
void set_spatial_mapping(untyped_image_base<N>& img,
    regular_grid<N,double> const& grid)
{
    assert(img.size == grid.n_points);
    set_spatial_mapping(img, grid.p0 - grid.spacing * 0.5, grid.spacing);
}

// This tests if an image's spatial mapping is axis aligned
// (i.e., image X is spatial X, image Y is spatial Y, etc.).
template<unsigned N>
bool is_axis_aligned(untyped_image_base<N> const& img)
{
    for (unsigned i = 0; i != N; ++i)
    {
        if (img.axes[i][i] <= 0)
            return false;
        for (unsigned j = 0; j != N; ++j)
        {
            if (j != i && !almost_equal(img.axes[i][j], 0.))
                return false;
        }
    }
    return true;
}

// This is similar to is_axis_aligned() but slightly weaker. It tests if
// each image axis is aligned with SOME spatial axis, but it doesn't care
// which one it is or whether or not the axis is inverted.
template<unsigned N>
bool is_orthogonal_to_axes(untyped_image_base<N> const& img)
{
    for (unsigned i = 0; i != N; ++i)
    {
        unsigned nonzeros = 0;
        for (unsigned j = 0; j != N; ++j)
        {
            if (!almost_equal(img.axes[i][j], 0.))
                ++nonzeros;
        }
        if (nonzeros != 1)
            return false;
    }
    return true;
}

template<unsigned N>
box<N,double> get_bounding_box(untyped_image_base<N> const& img)
{
    assert(is_orthogonal_to_axes(img));
    vector<N,double> origin = get_origin(img);
    box<N,double> box(
        origin,
        transform_point(get_spatial_mapping(img),
            vector<N,double>(img.size)) - origin);
    for (unsigned i = 0; i != N; ++i)
    {
        if (box.size[i] < 0)
        {
            box.corner[i] += box.size[i];
            box.size[i] = -box.size[i];
        }
    }
    return box;
}

template<unsigned N>
vector<N,double>
get_pixel_center(untyped_image_base<N> const& img,
    vector<N,unsigned> const& p)
{
    return transform_point(get_spatial_mapping(img),
        vector<N,double>(p) +
        uniform_vector<N,double>(0.5));
}

template<unsigned N>
vector<N,double> get_origin(untyped_image_base<N> const& img)
{
    return img.origin;
}

template<unsigned N>
vector<N,double> get_spacing(untyped_image_base<N> const& img)
{
    vector<N,double> spacing;
    for (unsigned i = 0; i < N; ++i)
        spacing[i] = length(img.axes[i]);
    return spacing;
}

template<unsigned N>
regular_grid<N,double> get_grid(untyped_image_base<N> const& img)
{
    regular_grid<N,double> grid;
    assert(is_axis_aligned(img));
    grid.p0 = get_pixel_center(img, uniform_vector<N,unsigned>(0));
    grid.spacing = get_spacing(img);
    grid.n_points = vector<N,unsigned>(img.size);
    return grid;
}

template<unsigned N, class Pixel>
void create_image_on_grid(image<N,Pixel,unique>& img,
    regular_grid<N,double> const& grid)
{
    create_image(img, grid.n_points);
    set_spatial_mapping(img, grid);
}

template<unsigned N, class Pixel>
image<N,Pixel,view> make_view_on_grid(
    Pixel* pixels, regular_grid<N,double> const& grid)
{
    image<N,Pixel,view> img(pixels, grid.n_points,
        get_contiguous_steps(grid.n_points));
    set_spatial_mapping(img, grid);
    return img;
}

template<unsigned N, class Pixel>
image<N,Pixel,const_view> make_const_view_on_grid(
    Pixel* pixels, regular_grid<N,double> const& grid)
{
    image<N,Pixel,const_view> img(pixels, grid.n_points,
        get_contiguous_steps(grid.n_points));
    set_spatial_mapping(img, grid);
    return img;
}

}

#endif
