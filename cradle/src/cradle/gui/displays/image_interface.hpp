#ifndef CRADLE_GUI_DISPLAYS_IMAGE_INTERFACE_HPP
#define CRADLE_GUI_DISPLAYS_IMAGE_INTERFACE_HPP


#include <cradle/gui/displays/types.hpp>

#include <alia/ui/api.hpp>
#include <cradle/geometry/common.hpp>
#include <cradle/geometry/slicing.hpp>
#include <cradle/imaging/inclusion_image.hpp>
#include <cradle/imaging/variant.hpp>
#include <cradle/gui/common.hpp>
#include <cradle/geometry/regular_grid.hpp>
#include <cradle/gui/requests.hpp>


// This file contains image interfaces which are designed to be passed to
// image visualization functions. These interfaces differ from the normal
// image interface in three important ways...
//
// * They are runtime polymorphic rather than compile-time polymorphic, which
//   means that the functions that accept these interfaces don't need to be
//   templated.
//
// * They support unevenly spaced images.
//
// * All queries work directly with UI accessors.

namespace cradle {

// All images, regardless of dimensionality, implement this interface.
struct any_image_interface
{
    virtual ~any_image_interface() {}

    // Get the min and max of the image.
    // request form
    virtual indirect_accessor<request<optional<min_max<double> > > >
    get_min_max_request(gui_context& ctx) const = 0;
    // value form
    gui_request_accessor<optional<min_max<double> > >
    get_min_max(gui_context& ctx) const
    {
        return gui_request(ctx, this->get_min_max_request(ctx));
    }

    // Get common statistics for the whole image.
    // request form
    virtual indirect_accessor<request<statistics<double> > >
    get_statistics_request(gui_context& ctx) const = 0;
    // value form
    gui_request_accessor<statistics<double> >
    get_statistics(gui_context& ctx) const
    {
        return gui_request(ctx, this->get_statistics_request(ctx));
    }

    // Get common statistics for a subset of the image.
    // request form
    virtual indirect_accessor<request<statistics<double> > >
    get_partial_statistics_request(
        gui_context& ctx,
        accessor<request<std::vector<weighted_grid_index> > > const& indices)
        const = 0;
    // value form
    gui_request_accessor<statistics<double> >
    get_partial_statistics(
        gui_context& ctx,
        accessor<request<std::vector<weighted_grid_index> > > const& indices)
        const
    {
        return gui_request(ctx,
            this->get_partial_statistics_request(ctx, indices));
    }

    // Get the range of possible values for this image.
    // This is used to determine the appropriate range for graphs over the
    // value space of the image. It must cover at least all the values in the
    // image. (It can cover more if the image is dynamic and you want to
    // maintain a stable value range as it changes.)
    // request form
    virtual indirect_accessor<request<optional<min_max<double> > > >
    get_value_range_request(gui_context& ctx) const = 0;
    // value form
    gui_request_accessor<optional<min_max<double> > >
    get_value_range(gui_context& ctx) const
    {
       return gui_request(ctx, this->get_value_range_request(ctx));
    }

    // Get a histogram of the image.
    // request form
    virtual indirect_accessor<request<image1> >
    get_histogram_request(
        gui_context& ctx,
        accessor<double> const& min_value,
        accessor<double> const& max_value,
        accessor<double> const& bin_size) const = 0;
    // value form
    gui_request_accessor<image1>
    get_histogram(
        gui_context& ctx,
        accessor<double> const& min_value,
        accessor<double> const& max_value,
        accessor<double> const& bin_size) const
    {
        return gui_request(ctx,
            this->get_histogram_request(ctx, min_value, max_value, bin_size));
    }

    // Get a histogram of a subset of the image.
    // request form
    virtual indirect_accessor<request<image1> >
    get_partial_histogram_request(
        gui_context& ctx,
        accessor<request<std::vector<weighted_grid_index> > > const& indices,
        accessor<double> const& min_value,
        accessor<double> const& max_value,
        accessor<double> const& bin_size) const = 0;
    // value form
    gui_request_accessor<image1>
    get_partial_histogram(
        gui_context& ctx,
        accessor<request<std::vector<weighted_grid_index> > > const& indices,
        accessor<double> const& min_value,
        accessor<double> const& max_value,
        accessor<double> const& bin_size) const
    {
        return gui_request(ctx,
            this->get_partial_histogram_request(ctx, indices, min_value,
                max_value, bin_size));
    }

    // Get the units of the pixel values.
    // request form
    virtual indirect_accessor<request<string> >
    get_value_units_request(gui_context& ctx) const = 0;
    // value form
    gui_request_accessor<string>
    get_value_units(gui_context& ctx) const
    {
        return gui_request(ctx, this->get_value_units_request(ctx));
    }
};




// Get a bounding box that includes both the irregular and the regularly
// spaced versions of an image.
api(fun internal with(N:1,2,3,4))
template<unsigned N>
// Bounding box of N size of image
box<N,double> bounding_box(
    // image_geometry<N> to compute bounding box of
    image_geometry<N> const& geometry)
{
    box<N,double> box = bounding_box(geometry.grid);
    for (unsigned i = 0; i != N; ++i)
    {
        if (!geometry.slicing[i].empty())
        {
            slice_description const& first =
                geometry.slicing[i].front();
            double lower_bound = first.position - first.thickness / 2;
            if (lower_bound < box.corner[i])
                 box.corner[i] = lower_bound;

            slice_description const& last =
                geometry.slicing[i].back();
            double upper_bound = last.position + last.thickness / 2;
            if (upper_bound > box.corner[i] + box.size[i])
                 box.size[i] = upper_bound - box.corner[i];
        }
    }
    return box;
}

// All N-dimensional images implement image_interface<N>.
template<unsigned N>
struct image_interface : any_image_interface
{
    // Get a description of the geometry of the image.
    // request form
    virtual indirect_accessor<request<image_geometry<N> > >
    get_geometry_request(gui_context& ctx) const = 0;
    // value form
    gui_request_accessor<image_geometry<N> >
    get_geometry(gui_context& ctx) const
    {
        return gui_request(ctx, this->get_geometry_request(ctx));
    }

    // Get the regularly spaced version of the image.
    // request form
    virtual indirect_accessor<request<image<N,variant,shared> > >
    get_regularly_spaced_image_request(gui_context& ctx) const = 0;
    // value form
    gui_request_accessor<image<N,variant,shared> >
    get_regularly_spaced_image(gui_context& ctx) const
    {
        return gui_request(ctx,
            this->get_regularly_spaced_image_request(ctx));
    }

    // Get the value of the image at the given point.
    // If the point is outside the image bounds, returns an empty value.
    // request form
    virtual indirect_accessor<request<optional<double> > >
    get_point_request(
        gui_context& ctx,
        accessor<request<vector<N,double> > > const& p) const = 0;
    // value form
    gui_request_accessor<optional<double> >
    get_point(
        gui_context& ctx,
        accessor<request<vector<N,double> > > const& p) const
    {
        return gui_request(ctx, this->get_point_request(ctx, p));
    }
};

// The following are the specializations for the individual dimensionalities.
// Generally, these are the types that are actually used in practice.

struct image_interface_1d : image_interface<1>
{
};

struct image_interface_2d : image_interface<2>
{
    // Get an interface to the line at the given position.
    // The caller should only provide positions that are in bounds.
    // Since the interface is not a concrete value, rather than returning an
    // accessor, this returns a pointer to the interface.
    virtual image_interface_1d const&
    get_line(
        gui_context& ctx,
        accessor<unsigned> const& axis,
        accessor<double> const& position) const = 0;
};

struct image_interface_3d : image_interface<3>
{
    // Get an interface to the slice at the given position.
    // (See image_interface_2d::get_line for comments.)
    virtual image_interface_2d const&
    get_slice(
        gui_context& ctx,
        accessor<unsigned> const& axis,
        accessor<double> const& position) const = 0;

    virtual indirect_accessor<request<std::vector<weighted_grid_index> > >
    get_voxels_in_structure_request(
        gui_context& ctx,
        accessor<request<structure_geometry> > const& geometry) const = 0;

    virtual indirect_accessor<request<double> >
        get_voxel_volume_scale(gui_context& ctx) const = 0;
};

}

#endif
