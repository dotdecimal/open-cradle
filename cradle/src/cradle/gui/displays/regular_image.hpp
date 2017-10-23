#ifndef CRADLE_GUI_DISPLAYS_REGULAR_IMAGE_HPP
#define CRADLE_GUI_DISPLAYS_REGULAR_IMAGE_HPP

#include <cradle/imaging/slicing.hpp>
#include <cradle/imaging/variant.hpp>
#include <cradle/gui/displays/image_interface.hpp>

// This file provides implementations of the GUI image interface for the
// standard CRADLE variant image types (image1, image2, image3).

namespace cradle {

// Given an accessor to an image with a known pixel type, this converts it to
// an accessor for a variant image.
// Since variant images and typed images are the same from an external
// perspective, and this is really just changing the C++ type of the accessor,
// it preserves the ID.
template<class Wrapped>
struct variant_accessor_wrapper
  : accessor<image<Wrapped::value_type::dimensionality,variant,shared> >
{
    typedef image<Wrapped::value_type::dimensionality,variant,shared>
        variant_type;
    variant_accessor_wrapper() {}
    variant_accessor_wrapper(Wrapped wrapped) : wrapped_(wrapped) {}
    bool is_gettable() const { return wrapped_.is_gettable(); }
    variant_type const& get() const { return lazy_getter_.get(*this); }
    id_interface const& id() const { return wrapped_.id(); }
    bool is_settable() const { return false; }
    void set(variant_type const& value) const {}
 private:
    friend struct lazy_getter<variant_type>;
    variant_type generate() const { return as_variant(wrapped_.get()); }
    Wrapped wrapped_;
    lazy_getter<variant_type> lazy_getter_;
};
template<class Wrapped>
variant_accessor_wrapper<Wrapped>
as_variant_accessor(Wrapped accessor)
{ return variant_accessor_wrapper<Wrapped>(accessor); }

// Compute the image geometry for an image.
api(fun internal with(N:1,2,3))
template<unsigned N>
// The image geometry of the given image
image_geometry<N>
compute_regular_image_geometry(
    // The image that will form as the base of the image geometry
    image<N,variant,shared> const& image,
    // Information about how the image fits into the next higher dimensional space
    optional<out_of_plane_information> const& oop_info)
{
    image_geometry<N> geometry;
    geometry.grid = get_grid(image);
    for (unsigned i = 0; i != N; ++i)
        geometry.slicing[i] = get_slices_for_grid(geometry.grid, i);
    geometry.out_of_plane_info = oop_info;
    return geometry;
}

// Get the default value range for an image.
// 1D version
indirect_accessor<request<optional<min_max<double> > > >
get_default_value_range(
    gui_context& ctx,
    accessor<request<image<1,variant,shared> > > const& img);
// 2D version
indirect_accessor<request<optional<min_max<double> > > >
get_default_value_range(
    gui_context& ctx,
    accessor<request<image<2,variant,shared> > > const& img);
// 3D version
indirect_accessor<request<optional<min_max<double> > > >
get_default_value_range(
    gui_context& ctx,
    accessor<request<image<3,variant,shared> > > const& img);

// Create an interface to an image.
// Note that everything is passed in as indirect accessors to allow copying.
// The accessors referenced by these must remain valid as long as the
// interface is in use.
// 1D version
image_interface_1d&
make_image_interface_unsafe(
    gui_context& ctx,
    indirect_accessor<request<image1> > image,
    indirect_accessor<request<optional<out_of_plane_information> > > oop_info,
    indirect_accessor<request<optional<min_max<double> > > > value_range);
// 2D version
image_interface_2d&
make_image_interface_unsafe(
    gui_context& ctx,
    indirect_accessor<request<image2> > image,
    indirect_accessor<request<optional<out_of_plane_information> > > oop_info,
    indirect_accessor<request<optional<min_max<double> > > > value_range);
// 3D version
image_interface_3d&
make_image_interface_unsafe(
    gui_context& ctx,
    indirect_accessor<request<image3> > image,
    indirect_accessor<request<optional<out_of_plane_information> > > oop_info,
    indirect_accessor<request<optional<min_max<double> > > > value_range);

// Create an interface to a image.
// This version is safer/more convenient since it takes care of ensuring that
// the accessors remain valid.
template<class Image, class OopInfo, class ValueRange>
auto
make_image_interface(
    gui_context& ctx,
    Image const& image,
    OopInfo const& oop_info,
    ValueRange const& value_range)
 -> decltype(
        make_image_interface_unsafe(ctx,
            make_indirect(ctx, image),
            make_indirect(ctx, oop_info),
            make_indirect(ctx, value_range)))
{
    return
        make_image_interface_unsafe(ctx,
            make_indirect(ctx, image),
            make_indirect(ctx, oop_info),
            make_indirect(ctx, value_range));
}

// Helper function
indirect_accessor<request<std::vector<weighted_grid_index> > >
get_default_voxels_in_structure_request(
    gui_context& ctx,
    image_interface_3d const* img,
    accessor<request<structure_geometry> > const& geometry);

// Computes the volume of a single grid cell which is used as a scale factor in
// determining a structure's volume with the cell inclusion info for regular grid images
indirect_accessor<request<double> >
get_default_image_scale_factor_request(
    gui_context& ctx,
    image_interface_3d const* img);

}

#endif
