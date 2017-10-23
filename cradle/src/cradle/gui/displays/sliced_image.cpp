#include <cradle/gui/displays/sliced_image.hpp>

#include <cradle/imaging/sample.hpp>
#include <cradle/imaging/api.hpp>
#include <cradle/gui/displays/regular_image.hpp>
#include <cradle/imaging/statistics.hpp>
#include <cradle/gui/displays/image_implementation.hpp>

namespace cradle {

template<unsigned N>
image_geometry<N + 1>
compute_generic_sliced_image_geometry(
    std::vector<image_slice<N,variant,shared> > const& slices)
{
    unsigned slice_axis = get_slice_axis(slices);
    image_geometry<N + 1> geometry;
    geometry.grid =
        unslice(
            get_image_grid(slices[0].content),
            2,
            compute_interpolation_grid(
                map(
                    [](image_slice<N,variant,shared> const& slice) -> double
                    { return slice.position; },
                    slices),
                4.));
    for (unsigned i = 0; i != N + 1; ++i)
    {
        if (i == slice_axis)
        {
            geometry.slicing[i] = extract_slice_descriptions(slices);
        }
        else
        {
            geometry.slicing[i] =
                get_slices_for_grid(geometry.grid, i);
        }
    }
    return geometry;
}

image_geometry<3>
compute_sliced_image_geometry(
    std::vector<image_slice<2,variant,shared> > const& slices)
{
    return compute_generic_sliced_image_geometry(slices);
}

template<unsigned N>
request<optional<min_max<double> > >
compose_sliced_image_min_max_request(
    request<std::vector<image_slice<N,variant,shared> > > const& slices)
{
    return rq_image_list_min_max(rq_extract_slice_images(slices));
}

template<unsigned N>
indirect_accessor<request<optional<min_max<double> > > >
get_sliced_image_min_max_request(gui_context& ctx,
    accessor<request<std::vector<image_slice<N,variant,shared> > > > const&
        slices)
{
    return make_indirect(ctx,
        gui_apply(ctx,
            compose_sliced_image_min_max_request<N>,
            slices));
}

template<unsigned N>
request<statistics<double> >
compose_sliced_image_statistics_request(
    request<std::vector<image_slice<N,variant,shared> > > const& slices)
{
    return rq_image_list_statistics(rq_extract_slice_images(slices));
}

template<unsigned N>
indirect_accessor<request<statistics<double> > >
get_sliced_image_statistics_request(gui_context& ctx,
    accessor<request<std::vector<image_slice<N,variant,shared> > > > const&
        slices)
{
    return make_indirect(ctx,
        gui_apply(ctx,
            compose_sliced_image_statistics_request<N>,
            slices));
}

template<unsigned N>
request<image_geometry<N + 1> >
compose_sliced_image_geometry_request(
    request<std::vector<image_slice<N,variant,shared> > > const& slices)
{
    return rq_foreground(rq_compute_sliced_image_geometry(slices));
}

template<unsigned N>
indirect_accessor<request<image_geometry<N + 1> > >
get_sliced_image_geometry_request(gui_context& ctx,
    accessor<request<std::vector<image_slice<N,variant,shared> > > > const&
        slices)
{
    return make_indirect(ctx,
        gui_apply(ctx, compose_sliced_image_geometry_request<N>, slices));
}

template<unsigned N>
request<optional<double> >
compose_sliced_image_point_request(
    request<std::vector<image_slice<N,variant,shared> > > const& slices,
    request<vector<N + 1,double> > const& p)
{
    return rq_sliced_image_sample(slices, p);
}

template<unsigned N>
indirect_accessor<request<optional<double> > >
get_sliced_image_point_request(gui_context& ctx,
    accessor<request<std::vector<image_slice<N,variant,shared> > > > const&
        slices,
    accessor<request<vector<N + 1,double> > > const& p)
{
    return make_indirect(ctx,
        gui_apply(ctx, compose_sliced_image_point_request<N>, slices, p));
}

template<unsigned N>
request<image<N + 1,variant,shared> >
compose_interpolated_image_request(
    request<std::vector<image_slice<N,variant,shared> > > const& slices)
{
    return rq_merge_slices(slices);
}

template<unsigned N>
indirect_accessor<request<image<N + 1,variant,shared> > >
get_interpolated_image_request(gui_context& ctx,
    accessor<request<std::vector<image_slice<N,variant,shared> > > > const&
        slices)
{
    return make_indirect(ctx,
        gui_apply(ctx, compose_interpolated_image_request<N>, slices));
}

template<unsigned N>
request<optional<min_max<double> > >
compose_sliced_image_value_range_request(
    request<std::vector<image_slice<N,variant,shared> > > const& slices)
{
    return rq_image_list_min_max(rq_extract_slice_images(slices));
}

template<unsigned N>
indirect_accessor<request<optional<min_max<double> > > >
get_sliced_image_value_range_request(gui_context& ctx,
    accessor<request<std::vector<image_slice<N,variant,shared> > > > const&
        slices)
{
    return make_indirect(ctx,
        gui_apply(ctx, compose_sliced_image_value_range_request<N>, slices));
}

template<unsigned N>
request<string>
compose_sliced_image_units_request(
    request<std::vector<image_slice<N,variant,shared> > > const& slices)
{
    return rq_sliced_image_units(slices);
}

template<unsigned N>
indirect_accessor<request<string> >
get_sliced_image_value_units_request(gui_context& ctx,
    accessor<request<std::vector<image_slice<N,variant,shared> > > > const&
        slices)
{
    return make_indirect(ctx,
        gui_apply(ctx, compose_sliced_image_units_request<N>, slices));
}

template<unsigned N>
request<unsigned>
compose_slice_axis_request(
    request<std::vector<image_slice<N,variant,shared> > > const& slices)
{
    return rq_get_slice_axis(slices);
}

template<unsigned N>
indirect_accessor<request<image<N + 1,variant,shared> > >
get_slice_axis_request(gui_context& ctx,
    accessor<request<std::vector<image_slice<N,variant,shared> > > > const&
        slices)
{
    return make_indirect(ctx,
        gui_apply(ctx, compose_slice_axis_request<N>, slices));
}

template<unsigned N>
request<optional<image_slice<N,variant,shared> > > static
compose_sliced_image_slice_request(
    request<std::vector<image_slice<N,variant,shared> > > const& slices,
    unsigned image_axis,
    unsigned slice_axis,
    double position)
{
    if (slice_axis == image_axis)
    {
        return rq_find_sliced_image_slice(slices, rq_value(position));
    }
    else
    {
        return rq_uninterpolated_image_slice(
            compose_interpolated_image_request(slices),
            rq_value(slice_axis), rq_value(position));
    }
}

template<unsigned N>
request<image<N,variant,shared> > static
compose_sliced_image_slice_image_request(
    request<std::vector<image_slice<N,variant,shared> > > const& slices,
    unsigned image_axis,
    unsigned slice_axis,
    double position)
{
    auto slice =
        compose_sliced_image_slice_request(slices, image_axis, slice_axis,
            position);
    return rq_add_empty_image_fallback(
        rq_optional_image_slice_content(slice));
}

template<unsigned N>
request<optional<out_of_plane_information> > static
compose_sliced_image_slice_oop_info_request(
    request<std::vector<image_slice<N,variant,shared> > > const& slices,
    unsigned image_axis,
    unsigned slice_axis,
    double position)
{
    auto slice =
        compose_sliced_image_slice_request(slices, image_axis, slice_axis,
            position);
    return rq_optional_image_slice_oop_info(slice);
}

#define CRADLE_IMPLEMENT_SLICED_IMAGE_INTERFACE(N, slices) \
    indirect_accessor<request<optional<min_max<double> > > > \
    get_min_max_request(gui_context& ctx) const \
    { return cradle::get_sliced_image_min_max_request(ctx, slices); } \
    \
    indirect_accessor<request<statistics<double> > > \
    get_statistics_request(gui_context& ctx) const \
    { return cradle::get_sliced_image_statistics_request(ctx, slices); } \
    \
    indirect_accessor<request<statistics<double> > > \
    get_partial_statistics_request(gui_context& ctx, \
        accessor<request<std::vector<weighted_grid_index> > > const& indices) \
        const \
    { \
        return cradle::get_partial_statistics_request(ctx, \
            interpolated_image, indices); \
    } \
    \
    indirect_accessor<request<optional<min_max<double> > > > \
    get_value_range_request(gui_context& ctx) const \
    { return cradle::get_sliced_image_min_max_request(ctx, slices); } \
    \
    indirect_accessor<request<image1> > \
    get_histogram_request(gui_context& ctx, \
        accessor<double> const& min_value, \
        accessor<double> const& max_value, \
        accessor<double> const& bin_size) const \
    { \
        return cradle::get_histogram_request(ctx, \
            interpolated_image, \
            min_value, max_value, bin_size); \
    } \
    \
    indirect_accessor<request<image1> > \
    get_partial_histogram_request(gui_context& ctx, \
        accessor<request<std::vector<weighted_grid_index> > > const& indices, \
        accessor<double> const& min_value, \
        accessor<double> const& max_value, \
        accessor<double> const& bin_size) const \
    { \
        return cradle::get_partial_histogram_request(ctx, \
            interpolated_image, indices, \
            min_value, max_value, bin_size); \
    } \
    \
    indirect_accessor<request<string> > \
    get_value_units_request(gui_context& ctx) const \
    { return cradle::get_sliced_image_value_units_request(ctx, slices); } \
    \
    indirect_accessor<request<image_geometry<N> > > \
    get_geometry_request(gui_context& ctx) const \
    { return cradle::get_sliced_image_geometry_request(ctx, slices); } \
    \
    indirect_accessor<request<optional<double> > > \
    get_point_request(gui_context& ctx, \
        accessor<request<vector<N,double> > > const& p) const \
    { return cradle::get_sliced_image_point_request(ctx, slices, p); }

// 3D sliced images
struct sliced_image_3d : image_interface_3d
{
    CRADLE_IMPLEMENT_SLICED_IMAGE_INTERFACE(3, slices)

    indirect_accessor<request<image<3,variant,shared> > >
    get_regularly_spaced_image_request(gui_context& ctx) const
    { return interpolated_image; }

    image_interface_2d const&
    get_slice(
        gui_context& ctx,
        accessor<unsigned> const& axis,
        accessor<double> const& position) const
    {
        auto value_range =
            get_sliced_image_value_range_request(ctx, slices);

        auto image_axis =
            gui_request(ctx,
                gui_apply(ctx, compose_slice_axis_request<2>, slices));

        auto image_request =
            make_indirect(ctx,
                gui_apply(ctx,
                    compose_sliced_image_slice_image_request<2>,
                    slices,
                    image_axis,
                    axis,
                    position));

        auto oop_info =
            make_indirect(ctx,
                gui_apply(ctx,
                    compose_sliced_image_slice_oop_info_request<2>,
                    slices,
                    image_axis,
                    axis,
                    position));

        return make_image_interface_unsafe(ctx, image_request, oop_info, value_range);
    }

    indirect_accessor<request<std::vector<weighted_grid_index> > >
    get_voxels_in_structure_request(
        gui_context& ctx,
        accessor<request<structure_geometry> > const& geometry) const
    {
        return get_default_voxels_in_structure_request(ctx, this, geometry);
    }

    indirect_accessor<request<double> >
    get_voxel_volume_scale(gui_context& ctx) const
    {
        return get_default_image_scale_factor_request(ctx, this);
    }

    indirect_accessor<request<image2_slice_list> > slices;
    indirect_accessor<request<image3> > interpolated_image;
};

image_interface_3d&
make_sliced_image_interface(
    gui_context& ctx,
    indirect_accessor<request<image2_slice_list> > slices)
{
    sliced_image_3d si3;
    si3.slices = slices;
    si3.interpolated_image =
        make_indirect(ctx,
            gui_apply(ctx,
                compose_interpolated_image_request<2>,
                slices));
    return *erase_type(ctx, si3);
}

}
