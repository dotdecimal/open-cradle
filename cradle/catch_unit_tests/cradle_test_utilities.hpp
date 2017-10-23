#ifndef CRADLE_TEST_UTILITIES_HPP
#define CRADLE_TEST_UTILITIES_HPP

#include <cradle/common.hpp>
#include <cradle/geometry/common.hpp>

using namespace cradle;

namespace unittests
{

const double tol = 0.001;

// Air Phantom Image
const double ph_image_corner = -10.0;
const double ph_image_length = 20.0;
const double ph_image_value = 0.01;
const double ph_pixel_spacing = 1.0;

// Cube structure w/ cube hole
const int numberOfSlices = 10;
const double slice_thickness = 1.0;
const double sq_corner = -6.0;
const double sq_length = 12.0;
const double hole_corner = -2.0;
const double hole_length = 4.0;
const double override_value = 0.5;
const double sq_start_Z_slice = -0.5;
const double sq_start_Z_position = sq_start_Z_slice - 0.5 * slice_thickness;
const double sq_end_Z_position = numberOfSlices * slice_thickness + sq_start_Z_position;
const double sq_start_xy_position = sq_corner + 0.5 * ph_pixel_spacing;
const double sq_end_xy_position = sq_length * ph_pixel_spacing + sq_start_xy_position;
const double hole_start_xy_position = hole_corner + 0.5 * ph_pixel_spacing;
const double hole_end_xy_position =
    hole_length * ph_pixel_spacing + hole_start_xy_position;

// Distance between two vectors
double
distance(vector3d p1, vector3d p2);

blob
readFileToBlob(string fileName);

}

#endif