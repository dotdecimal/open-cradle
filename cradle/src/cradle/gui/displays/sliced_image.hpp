#ifndef CRADLE_GUI_DISPLAYS_SLICED_IMAGE_HPP
#define CRADLE_GUI_DISPLAYS_SLICED_IMAGE_HPP

#include <cradle/gui/displays/image_interface.hpp>
#include <cradle/imaging/slicing.hpp>
#include <cradle/imaging/variant.hpp>

namespace cradle {

// Creates an image geometry from a list of image slices
api(fun internal)
// The image geometry of the given slice list
image_geometry<3>
compute_sliced_image_geometry(
    // The image slice list that will form as the base of the image geometry
    image2_slice_list const& slices);

// Create an interface to a 3D sliced image.
// Note that the image is passed in as an indirect accessor. The accessor that
// it refers to must remain valid as long as the interface is in use.
image_interface_3d&
make_sliced_image_interface(
    gui_context& ctx,
    indirect_accessor<request<image2_slice_list> > slices);

}

#endif
