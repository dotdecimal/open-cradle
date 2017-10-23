#ifndef CRADLE_GUI_DISPLAYS_IMAGE_IMPLEMENTATION_HPP
#define CRADLE_GUI_DISPLAYS_IMAGE_IMPLEMENTATION_HPP

#include <cradle/gui/displays/image_interface.hpp>
#include <cradle/imaging/api.hpp>

namespace cradle {

template<unsigned N>
request<statistics<double> >
compose_partial_statistics_request(
    request<image<N,variant,shared> > const& image,
    request<std::vector<weighted_grid_index> > const& indices)
{
    return rq_weighted_partial_image_statistics(image, indices);
}

template<unsigned N>
indirect_accessor<request<statistics<double> > >
get_partial_statistics_request(gui_context& ctx,
    accessor<request<image<N,variant,shared> > > const& img,
    accessor<request<std::vector<weighted_grid_index> > > const& indices)
{
    return make_indirect(ctx,
        gui_apply(ctx, compose_partial_statistics_request<N>, img, indices));
}

template<unsigned N>
request<image1>
compose_histogram_request(
    request<image<N,variant,shared> > const& image,
    double const& min_value,
    double const& max_value,
    double const& bin_size)
{
    return rq_image_histogram(image, rq_value(min_value), rq_value(max_value),
        rq_value(bin_size));
}

template<unsigned N>
indirect_accessor<request<image1> >
get_histogram_request(gui_context& ctx,
    accessor<request<image<N,variant,shared> > > const& img,
    accessor<double> const& min_value,
    accessor<double> const& max_value,
    accessor<double> const& bin_size)
{
    return make_indirect(ctx,
        gui_apply(ctx, compose_histogram_request<N>,
            img, min_value, max_value, bin_size));
}

template<unsigned N>
request<image1>
compose_partial_histogram_request(
    request<image<N,variant,shared> > const& image,
    request<std::vector<weighted_grid_index> > const& indices,
    double const& min_value,
    double const& max_value,
    double const& bin_size)
{
    return rq_partial_image_histogram(
        image, indices, rq_value(min_value), rq_value(max_value),
        rq_value(bin_size));
}

template<unsigned N>
indirect_accessor<request<image1> >
get_partial_histogram_request(gui_context& ctx,
    accessor<request<image<N,variant,shared> > > const& img,
    accessor<request<std::vector<weighted_grid_index> > > const& indices,
    accessor<double> const& min_value,
    accessor<double> const& max_value,
    accessor<double> const& bin_size)
{
    return make_indirect(ctx,
        gui_apply(ctx,
            compose_partial_histogram_request<N>,
            img, indices, min_value, max_value, bin_size));
}

// Get the out-of-plane information for an image slice.
api(fun trivial internal with(N:1,2,3))
template<unsigned N>
optional<out_of_plane_information>
image_slice_oop_info(image_slice<N,variant,shared> const& slice)
{
    return out_of_plane_information(
        slice.axis, slice.thickness, slice.position);
}

api(fun trivial internal with(N:1,2,3))
template<unsigned N>
optional<out_of_plane_information>
optional_image_slice_oop_info(
    optional<image_slice<N,variant,shared> > const& slice)
{
    if (slice)
        return image_slice_oop_info(get(slice));
    else
        return none;
}

api(fun trivial internal with(N:1,2,3))
template<unsigned N>
optional<image<N,variant,shared> >
optional_image_slice_content(
    optional<image_slice<N,variant,shared> > const& slice)
{
    if (slice)
        return get(slice).content;
    else
        return none;
}

template<unsigned N>
request<optional<out_of_plane_information> >
compose_oop_info_request(request<image_slice<N,variant,shared> > const& slice)
{
    return rq_image_slice_oop_info(slice);
}

template<unsigned N>
indirect_accessor<request<optional<out_of_plane_information> > >
get_oop_info_request(gui_context& ctx,
    accessor<request<image_slice<N,variant,shared> > > const& slice)
{
    return make_indirect(ctx,
        gui_apply(ctx, compose_oop_info_request<N>, slice));
}

// If the argument has a value, return that. Otherwise, return an empty image.
// This is used to make the types work out for certain requests.
api(fun trivial internal with(N:1,2,3))
template<unsigned N>
image<N,variant,shared>
add_empty_image_fallback(optional<image<N,variant,shared> > const& img)
{
    if (img)
        return get(img);
    else
        return as_variant(empty_image<N,uint8_t>());
}

}

#endif
