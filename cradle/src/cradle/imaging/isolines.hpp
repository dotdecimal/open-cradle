#ifndef CRADLE_IMAGING_ISOLINES_HPP
#define CRADLE_IMAGING_ISOLINES_HPP

#include <cradle/geometry/common.hpp>
#include <cradle/imaging/image.hpp>
#include <vector>

namespace cradle {

// Given a grayscale image and a level, this will create a list of line
// segments that divide the image into regions above and below the level.
api(fun with(Pixel:variant;Storage:shared))
template<class Pixel, class Storage>
// The list of line segments that divide the image into regions above and below the given level.
std::vector<line_segment<2,double> >
compute_isolines(
    // The image that the isolines will divide.
    image<2,Pixel,Storage> const& img,
    // The level that will determine where the isolines will be placed on the image.
    double level);

// If the edge of the image is also divided by the threshold level, the above
// functions will only produce a line segment that extends to the edge of the
// image. The contour won't extend around the edge of the image.
// Calling this function will extend along the edge of the image to close the
// contour. The contour is closed in such a way that the interior includes
// values ABOVE the level.
template<class Pixel, class Storage>
void close_isoline_contours(
    std::vector<line_segment<2,double> >& lines,
    image<2,Pixel,Storage> const& img, double level);

}

#include <cradle/imaging/isolines.ipp>

#endif
