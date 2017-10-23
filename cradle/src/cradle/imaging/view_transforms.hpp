#ifndef CRADLE_IMAGING_VIEW_TRANSFORMS_HPP
#define CRADLE_IMAGING_VIEW_TRANSFORMS_HPP

#include <cradle/imaging/variant.hpp>
#include <cradle/imaging/geometry.hpp>

namespace cradle {

// Creates a view of an image where the spatial position of the image has been
// transformed by the given transformation matrix.
template<unsigned N, class T, class SP>
image<N,T,SP> transformed_view(image<N,T,SP> const& img,
    matrix<N+1,N+1,double> const& transformation)
{
    image<N,T,SP> r = img;
    r.origin = transform_point(transformation, r.origin);
    for (unsigned i = 0; i != N; ++i)
        r.axes[i] = transform_vector(transformation, r.axes[i]);
    return r;
}

// A flipped_view of an image is a view of that image with the spatial mapping
// manipulated such that it will appear flipped along the specified axis.
template<unsigned N, class T, class SP>
image<N,T,SP> flipped_view(image<N,T,SP> const& img, unsigned axis)
{
    return transformed_view(img, scaling_transformation(
        unslice(uniform_vector<N-1,double>(1), axis, -1)));
}

// Invert an axis on an image such that the pixels are in the same spatial
// position but are in the reverse order.
template<unsigned N, class T, class SP>
void invert_axis(image<N,T,SP>& img, unsigned axis)
{
    img.pixels += img.step[axis] * (img.size[axis] - 1);
    img.step[axis] = -img.step[axis];
    img.origin += img.axes[axis] * img.size[axis];
    img.axes[axis] = -img.axes[axis];
}
template<unsigned N, class SP>
void invert_axis(image<N,variant,SP>& img, unsigned axis);

// Given an image whose axes are aligned with spatial axes but possibly
// mismatched or inverted (e.g., image X is aligned with spatial -Z), this
// returns a view in which the axes are matched up properly and not inverted.
template<unsigned N, class T, class SP>
image<N,T,SP> aligned_view(image<N,T,SP> const& img)
{
    assert(is_orthogonal_to_axes(img));
    image<N,T,SP> r = img;
    for (unsigned i = 0; i != N; ++i)
    {
        unsigned corresponding_axis = 0;
        for (unsigned j = 0; j != N; ++j)
        {
            if (!almost_equal(r.axes[j][i], 0.))
            {
                swap_axes(r, i, j);
                break;
            }
        }
        if (r.axes[i][i] < 0)
            invert_axis(r, i);
    }
    assert(is_axis_aligned(r));
    return r;
}
// same, but for variant images
template<unsigned N, class SP>
image<N,variant,SP> aligned_view(image<N,variant,SP> const& img);

// A raw_flipped_view of an image simply manipulates the pixel pointers so
// that the underlying pixels are flipped. It doesn't change the geometry of
// the image. The image will still occupy the same box in space, but its
// pixels will be flipped.
template<unsigned N, class T, class SP>
image<N,T,SP> raw_flipped_view(image<N,T,SP> const& img, unsigned axis)
{
    image<N,T,SP> r = img;
    r.pixels += r.step[axis] * (r.size[axis] - 1);
    r.step[axis] = -r.step[axis];
    return r;
}
// same, but for variant images
template<unsigned N, class SP>
image<N,variant,SP> raw_flipped_view(image<N,variant,SP> const& img, unsigned axis);

// The following functions provide raw rotated views of an image in increments
// of 90 degrees. Like raw_flipped_view, the rotations only affect the pixels,
// not the spatial mapping.
template<class T, class SP>
image<2,T,SP> raw_rotated_180_view(image<2,T,SP> const& img)
{
    image<2,T,SP> r = img;
    r.pixels += r.step[1] * (r.size[1] - 1) + r.step[0] * (r.size[0] - 1);
    r.step = -r.step;
    return r;
}
// same, but for variant images
template<class SP>
void raw_rotated_180_view(image<2,variant,SP>& img);

template<class T, class SP>
image<2,T,SP> raw_rotated_90ccw_view(image<2,T,SP> const& img)
{
    image<2,T,SP> r = img;
    r.pixels += r.step[0] * (r.size[0] - 1);
    r.step[0] = img.step[1];
    r.step[1] = -img.step[0];
    return r;
}
// same, but for variant images
template<class SP>
void raw_rotated_90ccw_view(image<2,variant,SP>& img);

template<class T, class SP>
image<2,T,SP> raw_rotated_90cw_view(image<2,T,SP> const& img)
{
    image<2,T,SP> r = img;
    r.pixels += r.step[1] * (r.size[1] - 1);
    r.step[1] = img.step[0];
    r.step[0] = -img.step[1];
    return r;
}
// same, but for variant images
template<class SP>
void raw_rotated_90cw_view(image<2,variant,SP>& img);

// subimage_view() returns a view of a subregion of an image.
template<unsigned N, class T, class SP>
image<N,T,SP> subimage_view(image<N,T,SP> const& img,
    box<N,unsigned> const& region)
{
    image<N,T,SP> r = img;
    for (unsigned i = 0; i < N; ++i)
        r.pixels += region.corner[i] * img.step[i];
    r.size = region.size;
    for (unsigned i = 0; i < N; ++i)
        r.origin += r.axes[i] * region.corner[i];
}
// same, but for variant images
template<unsigned N, class SP>
image<N,variant,SP> subimage_view(image<N,variant,SP> const& img,
    box<N,unsigned> const& region);

// subsampled_view() returns a view of an image in which the step size has
// been increased so that only a fraction of the pixels from the original
// image are included in the resulting view.
template<unsigned N, class T, class SP>
image<N,T,SP> subsampled_view(image<N,T,SP> const& img,
    vector<N,unsigned> const& step)
{
    image<N,T,SP> r = img;
    for (unsigned i = 0; i < N; ++i)
    {
        r.size[i] = (r.size[i] + (step[i] - 1)) / step[i];
        r.step[i] *= step[i];
        r.axes[i] *= step[i];
    }
    return r;
}
// same, but for variant images
template<unsigned N, class SP>
image<N,variant,SP> subsampled_view(image<N,variant,SP> const& img,
    vector<N,unsigned> const& step);

}

#include <cradle/imaging/view_transforms.ipp>

#endif
