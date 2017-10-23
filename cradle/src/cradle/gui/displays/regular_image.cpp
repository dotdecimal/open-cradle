#include <cradle/gui/displays/regular_image.hpp>

#include <cradle/gui/displays/image_interface.hpp>
#include <cradle/imaging/sample.hpp>
#include <cradle/imaging/api.hpp>
#include <cradle/imaging/variant.hpp>
#include <cradle/gui/displays/image_implementation.hpp>

#ifdef interface
#undef interface
#endif

namespace cradle {

// The following are general utilities for implementing the interfaces.

template<unsigned N>
request<optional<min_max<double> > > static
compose_min_max_request(request<image<N,variant,shared> > const& image)
{
    return rq_image_min_max(image);
}

template<unsigned N>
indirect_accessor<request<optional<min_max<double> > > >
get_min_max_request(gui_context& ctx,
    accessor<request<image<N,variant,shared> > > const& img)
{
    return make_indirect(ctx,
        gui_apply(ctx, compose_min_max_request<N>, img));
}

template<unsigned N>
request<statistics<double> >
compose_statistics_request(request<image<N,variant,shared> > const& image)
{
    return rq_image_statistics(image);
}

template<unsigned N>
indirect_accessor<request<statistics<double> > >
get_statistics_request(gui_context& ctx,
    accessor<request<image<N,variant,shared> > > const& img)
{
    return make_indirect(ctx,
        gui_apply(ctx, compose_statistics_request<N>, img));
}

template<unsigned N>
request<string>
compose_value_units_request(request<image<N,variant,shared> > const& image)
{
    return rq_image_value_units(image);
}

template<unsigned N>
indirect_accessor<request<string> >
get_value_units_request(gui_context& ctx,
    accessor<request<image<N,variant,shared> > > const& img)
{
    return make_indirect(ctx,
        gui_apply(ctx, compose_value_units_request<N>, img));
}

template<unsigned N>
request<image_geometry<N> >
compose_geometry_request(
    request<image<N,variant,shared> > const& image,
    request<optional<out_of_plane_information> > const& oop_info)
{
    return rq_compute_regular_image_geometry(image, oop_info);
}

template<unsigned N>
indirect_accessor<request<image_geometry<N> > >
get_geometry_request(gui_context& ctx,
    accessor<request<image<N,variant,shared> > > const& img,
    accessor<request<optional<out_of_plane_information> > > const& oop_info)
{
    return make_indirect(ctx,
        gui_apply(ctx, compose_geometry_request<N>, img, oop_info));
}

template<unsigned N>
request<optional<double> >
compose_point_value_request(
    request<image<N,variant,shared> > const& image,
    request<vector<N,double> > const& p)
{
    return rq_image_sample(image, p);
}

template<unsigned N>
indirect_accessor<request<optional<double> > >
get_point_request(gui_context& ctx,
    accessor<request<image<N,variant,shared> > > const& img,
    accessor<request<vector<N,double> > > const& p)
{
    return make_indirect(ctx,
        gui_apply(ctx, compose_point_value_request<N>, img, p));
}

template<unsigned N>
request<image_slice<N - 1,variant,shared> >
compose_requred_image_slice_request(
    request<image<N,variant,shared> > const& image,
    unsigned const& axis,
    double const& position)
{
    return rq_required(rq_uninterpolated_image_slice(
        image, rq_value(axis), rq_value(position)));
}

template<unsigned N>
request<image<N,variant,shared> >
compose_slice_content_request(
    request<image_slice<N,variant,shared> > const& slice)
{
    return rq_property(slice, content);
}

#define CRADLE_IMPLEMENT_REGULAR_IMAGE_INTERFACE(N) \
    indirect_accessor<request<optional<min_max<double> > > > \
    get_min_max_request(gui_context& ctx) const \
    { return cradle::get_min_max_request(ctx, img); } \
    \
    indirect_accessor<request<statistics<double> > > \
    get_statistics_request(gui_context& ctx) const \
    { return cradle::get_statistics_request(ctx, img); } \
    \
    indirect_accessor<request<statistics<double> > > \
    get_partial_statistics_request(gui_context& ctx, \
        accessor<request<std::vector<weighted_grid_index> > > const& indices) \
        const \
    { return cradle::get_partial_statistics_request(ctx, img, indices); } \
    \
    indirect_accessor<request<optional<min_max<double> > > > \
    get_value_range_request(gui_context& ctx) const \
    { return value_range; } \
    \
    indirect_accessor<request<image1> > \
    get_histogram_request(gui_context& ctx, \
        accessor<double> const& min_value, \
        accessor<double> const& max_value, \
        accessor<double> const& bin_size) const \
    { \
        return cradle::get_histogram_request(ctx, img, min_value, max_value, \
            bin_size); \
    } \
    \
    indirect_accessor<request<image1> > \
    get_partial_histogram_request(gui_context& ctx, \
        accessor<request<std::vector<weighted_grid_index> > > const& indices, \
        accessor<double> const& min_value, \
        accessor<double> const& max_value, \
        accessor<double> const& bin_size) const \
    { \
        return cradle::get_partial_histogram_request(ctx, img, indices, \
            min_value, max_value, bin_size); \
    } \
    \
    indirect_accessor<request<string> > \
    get_value_units_request(gui_context& ctx) const \
    { return cradle::get_value_units_request(ctx, img); } \
    \
    indirect_accessor<request<image_geometry<N> > > \
    get_geometry_request(gui_context& ctx) const \
    { return cradle::get_geometry_request(ctx, img, oop_info); } \
    \
    indirect_accessor<request<optional<double> > > \
    get_point_request(gui_context& ctx, \
        accessor<request<vector<N,double> > > const& p) const \
    { return cradle::get_point_request(ctx, img, p); }

// 1D images

indirect_accessor<request<optional<min_max<double> > > >
get_default_value_range(
    gui_context& ctx,
    accessor<request<image<1,variant,shared> > > const& img)
{
    return get_min_max_request(ctx, img);
}

struct regular_image_1d : image_interface_1d
{
    CRADLE_IMPLEMENT_REGULAR_IMAGE_INTERFACE(1)

    indirect_accessor<request<image<1,variant,shared> > >
    get_regularly_spaced_image_request(gui_context& ctx) const
    { return img; }

    indirect_accessor<request<image1> > img;
    indirect_accessor<request<optional<out_of_plane_information> > > oop_info;
    indirect_accessor<request<optional<min_max<double> > > > value_range;
};

image_interface_1d&
make_image_interface_unsafe(
    gui_context& ctx,
    indirect_accessor<request<image1> > image,
    indirect_accessor<request<optional<out_of_plane_information> > > oop_info,
    indirect_accessor<request<optional<min_max<double> > > > value_range)
{
    regular_image_1d interface;
    interface.img = image;
    interface.oop_info = oop_info;
    interface.value_range = value_range;
    return *erase_type(ctx, interface);
}

// 2D images

indirect_accessor<request<optional<min_max<double> > > >
get_default_value_range(
    gui_context& ctx,
    accessor<request<image<2,variant,shared> > > const& img)
{
    return get_min_max_request(ctx, img);
}

struct regular_image_2d : image_interface_2d
{
    CRADLE_IMPLEMENT_REGULAR_IMAGE_INTERFACE(2)

    indirect_accessor<request<image<2,variant,shared> > >
    get_regularly_spaced_image_request(gui_context& ctx) const
    { return img; }

    image_interface_1d const&
    get_line(
        gui_context& ctx,
        accessor<unsigned> const& axis,
        accessor<double> const& position) const
    {
        auto slice =
            gui_apply(ctx,
                compose_requred_image_slice_request<2>,
                img, axis, position);
        return
            make_image_interface_unsafe(
                ctx,
                make_indirect(ctx,
                    gui_apply(ctx, compose_slice_content_request<1>, slice)),
                get_oop_info_request(ctx, slice),
                value_range);
    }

    indirect_accessor<request<image2> > img;
    indirect_accessor<request<optional<out_of_plane_information> > > oop_info;
    indirect_accessor<request<optional<min_max<double> > > > value_range;
};

image_interface_2d&
make_image_interface_unsafe(
    gui_context& ctx,
    indirect_accessor<request<image2> > image,
    indirect_accessor<request<optional<out_of_plane_information> > > oop_info,
    indirect_accessor<request<optional<min_max<double> > > > value_range)
{
    regular_image_2d interface;
    interface.img = image;
    interface.oop_info = oop_info;
    interface.value_range = value_range;
    return *erase_type(ctx, interface);
}

// 3D eager images

indirect_accessor<request<optional<min_max<double> > > >
get_default_value_range(
    gui_context& ctx,
    accessor<request<image<3,variant,shared> > > const& img)
{
    return get_min_max_request(ctx, img);
}

// Note: The compiler seems to have a bug with trying to write this function
//       as a lambda, so it was defined fully instead
request<std::vector<weighted_grid_index> > static
voxels_in_structure_request_helper(
    request<image_geometry<3> > const& img_geom,
    request<structure_geometry> const& str_geometry)
{
    auto reg_grid = rq_property(img_geom, grid);
    auto cell_info = rq_compute_grid_cells_in_structure(reg_grid, str_geometry);
    return rq_property(cell_info, cells_inside);
}

indirect_accessor<request<std::vector<weighted_grid_index> > >
get_default_voxels_in_structure_request(
    gui_context& ctx,
    image_interface_3d const* img,
    accessor<request<structure_geometry> > const& geometry)
{
    return
        make_indirect(ctx,
            gui_apply(ctx,
                voxels_in_structure_request_helper,
                img->get_geometry_request(ctx),
                geometry));
}

indirect_accessor<request<double> >
get_default_image_scale_factor_request(
    gui_context& ctx,
    image_interface_3d const* img)
{
    return
        make_indirect(ctx,
            gui_apply(ctx,
                [](request<image_geometry<3> > const& img_geom)
                {
                    return rq_regular_grid_voxel_volume(rq_property(img_geom, grid));
                },
                img->get_geometry_request(ctx)));
}

struct regular_image_3d : image_interface_3d
{
    CRADLE_IMPLEMENT_REGULAR_IMAGE_INTERFACE(3)

    indirect_accessor<request<image<3,variant,shared> > >
    get_regularly_spaced_image_request(gui_context& ctx) const
    { return img; }

    image_interface_2d const&
    get_slice(
        gui_context& ctx,
        accessor<unsigned> const& axis,
        accessor<double> const& position) const
    {
        auto slice =
            gui_apply(ctx,
                compose_requred_image_slice_request<3>,
                img, axis, position);
        return
            make_image_interface_unsafe(
                ctx,
                make_indirect(ctx,
                    gui_apply(ctx, compose_slice_content_request<2>, slice)),
                get_oop_info_request(ctx, slice),
                value_range);
    }

    indirect_accessor<request<std::vector<weighted_grid_index> > >
    get_voxels_in_structure_request(
        gui_context& ctx,
        accessor<request<structure_geometry> > const& geometry) const
    {
        return
            get_default_voxels_in_structure_request(ctx, this, geometry);
    }

    indirect_accessor<request<double> >
    get_voxel_volume_scale(gui_context& ctx) const
    {
        return get_default_image_scale_factor_request(ctx, this);
    }

    indirect_accessor<request<image3> > img;
    indirect_accessor<request<optional<out_of_plane_information> > > oop_info;
    indirect_accessor<request<optional<min_max<double> > > > value_range;
};

image_interface_3d&
make_image_interface_unsafe(
    gui_context& ctx,
    indirect_accessor<request<image3> > image,
    indirect_accessor<request<optional<out_of_plane_information> > > oop_info,
    indirect_accessor<request<optional<min_max<double> > > > value_range)
{
    regular_image_3d interface;
    interface.img = image;
    interface.oop_info = oop_info;
    interface.value_range = value_range;
    return *erase_type(ctx, interface);
}

}
