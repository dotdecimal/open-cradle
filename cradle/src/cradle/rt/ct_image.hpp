#ifndef CRADLE_RT_CT_IMAGE_HPP
#define CRADLE_RT_CT_IMAGE_HPP

#include <cradle/api.hpp>
#include <cradle/common.hpp>
#include <cradle/imaging/slicing.hpp>
#include <cradle/imaging/variant.hpp>
#include <cradle/rt/common.hpp>

namespace cradle {

// Structure to hold image slice data for ct images in blob form
api(struct)
struct pixel_blob
{
    // Blob that holds ct image slice data
    cradle::blob blob;
};

// Constructs a pixel blob from an image2
api(fun)
// pixel_blob convented from image2
pixel_blob
construct_image_pixel_blob(image<2, double, shared> img)
{
    pixel_blob pb;
    auto x = as_variant(img);
    blob b;
    b.ownership = x.pixels.ownership;
    b.data = x.pixels.view;
    b.size =
        product(x.size) * get_channel_size(x.pixels.type_info.type) *
        get_channel_count(x.pixels.type_info.format);
    pb.blob = b;
    return pb;
}

// Different ways of storing image data for CT Images.
api(union)
union ct_image_data
{
    // image of N size
    pixel_blob pixel;
    // image of N size
    object_reference<pixel_blob> pixel_ref;
    // image of N size
    image<2, double, shared> img;
};

// Ct image dicom data
api(struct)
struct ct_image_slice_data
{
    // CT Image data with rescale applied
    ct_image_data img;
    // Number of bits allocated for each pixel
    int bits_allocated;
    // Number of bits stored for each pixel
    int bits_stored;
    // Most significant bit for pixel sample data
    int high_bit;
    // B value in rescale intercept equation
    double rescale_intercept;
    // M value in rescale intercept equation
    double rescale_slope;
    // Number of columns of pixels in the images
    int cols;
    // Number of rows of pixels in the images
    int rows;
};

// CT image slice dicom data
api(struct)
struct ct_image_slice_content
{
    // ct image
    ct_image_slice_data content;
    // Image slice axis (0:X, 1:Y, 2:Z)
    unsigned axis;
    // Image slice position along the axis direction.
    double position;
    // Image slice thickness along the axis direction.
    double thickness;
    // Instance number.
    int instance_number;
    // Number of samples in the image (e.g. monochrome = 1, rbg = 3)
    // (This is always 1 because our image type is "double")
    int samples_per_pixel;
    // Data representation of the pixel samples
    int pixel_rep;
    // Distance between the center of each pixel
    vector2d pixel_spacing;
    // XYZ Position of the first image pixel (upper left corner)
    vector3d image_position;
    // Direction cosines of the first row and the first column of data with respect to the
    // patient
    std::vector<double> image_orientation;
    // Intended interpretation of the pixel data (e.g. monochrome1 or monochrome2,
    // [palette color, rgb etc, are not supported])
    string photometric_interpretation;
};

// Stores individual image slice data.
api(struct)
struct ct_image_slice : dicom_file
{
    // Series information
    dicom_general_series series;
    // CT Image slice data
    ct_image_slice_content slice;
    // list of referenced uids
    std::vector<ref_dicom_item> referenced_ids;

};

// Stores image set of dicom data.
api(struct)
struct ct_image_set : dicom_file
{
    // Series information
    dicom_general_series series;
    // CT Image
    image3 image;

};

// Union for storing CT Images, either as a set or vector of slices
api(union)
union ct_image
{
    // CT Images stored as an image3
    ct_image_set image_set;
    // A slice of a ct image
    std::vector<ct_image_slice> image_slices;
};

}

#endif
