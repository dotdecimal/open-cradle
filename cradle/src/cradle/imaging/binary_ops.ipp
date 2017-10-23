#include <cradle/imaging/iterator.hpp>
#include <cradle/imaging/sample.hpp>
#include <cradle/imaging/blend.hpp>
#include <cradle/geometry/intersection.hpp>
#include <cradle/geometry/grid_points.hpp>

namespace cradle {

// GENERAL BINARY OPS

template<unsigned N, class Pixel1, class Storage1,
    class Pixel2, class Storage2>
bool calculate_common_grid(
    regular_grid<N,double>* grid,
    image<N,Pixel1,Storage1> const& img1,
    image<N,Pixel2,Storage2> const& img2)
{
    box<N,double> box1 = get_bounding_box(img1);
    box<N,double> box2 = get_bounding_box(img2);
    optional<box<N,double> > common_box =
        intersection(box1, box2);
    if (!common_box)
        return false;

    vector<N,double> spacing1 = get_spacing(img1);
    vector<N,double> spacing2 = get_spacing(img2);
    vector<N,double> spacing;
    for (unsigned i = 0; i < N; ++i)
        spacing[i] = (std::min)(spacing1[i], spacing2[i]);

    create_grid_for_box(grid, common_box.get(), spacing);
    return true;
}

template<unsigned N, class Pixel1, class Storage1,
    class Pixel2, class Storage2, class Op>
image<N,double,shared>
compute_binary_op(
    image<N,Pixel1,Storage1> const& img1,
    image<N,Pixel2,Storage2> const& img2,
    Op& op)
{
    // TODO: Add fast paths when grids and types align.

    regular_grid<N,double> common_grid;
    if (!calculate_common_grid(&common_grid, img1, img2))
        return image<N,double,shared>();

    image<N,double,unique> tmp;
    create_image_on_grid(tmp, common_grid);

    image_iterator<N,double,unique> result_i = get_begin(tmp);
    image_iterator<N,double,unique> result_end = get_end(tmp);

    regular_grid_point_list<N,double> points(common_grid);
    typename regular_grid_point_list<N,double>::const_iterator
        point_i = points.begin();

    for (; result_i != result_end; ++result_i, ++point_i)
    {
        optional<double> sample1 = image_sample(img1, *point_i);
        assert(sample1);
        optional<double> sample2 = image_sample(img2, *point_i);
        assert(sample2);
        *result_i = op(sample1.get(), sample2.get());
    }

    return share(tmp);
}

template<unsigned N, class Pixel1, class Storage1,
    class Pixel2, class Storage2, class Op>
void reduce_binary_op(
    image<N,Pixel1,Storage1> const& img1,
    image<N,Pixel2,Storage2> const& img2,
    Op& op)
{
    // TODO: Add fast path for when grids and types align.

    regular_grid<N,double> common_grid;
    if (!calculate_common_grid(&common_grid, img1, img2))
    {
        return;
    }

    regular_grid_point_list<N,double> points(common_grid);
    typename regular_grid_point_list<N,double>::const_iterator
        point_i = points.begin();
    typename regular_grid_point_list<N,double>::const_iterator
        point_end = points.end();

    for (; point_i != point_end; ++point_i)
    {
        optional<double> sample1 = image_sample(img1, *point_i);
        assert(sample1);
        optional<double> sample2 = image_sample(img2, *point_i);
        assert(sample2);
        op(sample1.get(), sample2.get());
    }
}

// SUM, WEIGHTED SUM

struct sum_op
{
    double operator()(double a, double b) { return a + b; }
};

template<unsigned N, class Pixel1, class Storage1,
    class Pixel2, class Storage2>
image<N,double,shared>
compute_sum(
    image<N,Pixel1,Storage1> const& img1,
    image<N,Pixel2,Storage2> const& img2)
{
    check_matching_units(img1.units, img2.units);
    sum_op op;
    image<N,double,shared> result = compute_binary_op(img1, img2, op);
    result.units = img1.units;
    return result;
}

struct weighted_sum_op
{
    double weight1, weight2;
    double operator()(double a, double b)
    { return weight1 * a + weight2 * b; }
};

template<unsigned N, class Pixel1, class Storage1,
    class Pixel2, class Storage2>
image<N,double,shared>
compute_weighted_sum(
    image<N,Pixel1,Storage1> const& img1, double weight1,
    image<N,Pixel2,Storage2> const& img2, double weight2)
{
    check_matching_units(img1.units, img2.units);
    weighted_sum_op op;
    op.weight1 = weight1;
    op.weight2 = weight2;
    image<N,double,shared> result = compute_binary_op(img1, img2, op);
    result.units = img1.units;
    return result;
}

template<unsigned N>
image<N,variant,shared>
sum_image_list(std::vector<image<N,variant,shared> > const& images)
{
    size_t n_images = images.size();
    switch (n_images)
    {
     case 0:
        throw exception("empty image list");
     case 1:
        return images[0];
     default:
      {
        auto sum = blend_images(images[0], images[1], 1, 1);
        for (size_t i = 2; i < n_images; ++i)
            sum = blend_images(sum, images[i], 1, 1);
        return as_variant(sum);
      }
    }
}

// DIFFERENCE

struct difference_op
{
    double operator()(double a, double b) { return a - b; }
};

template<unsigned N, class Pixel1, class Storage1,
    class Pixel2, class Storage2>
image<N,double,shared>
compute_difference(
    image<N,Pixel1,Storage1> const& img1,
    image<N,Pixel2,Storage2> const& img2)
{
    check_matching_units(img1.units, img2.units);
    difference_op op;
    image<N,double,shared> result = compute_binary_op(img1, img2, op);
    result.units = img1.units;
    return result;
}

struct max_difference_op
{
    double max;
    void operator()(double a, double b)
    {
        double diff = a - b;
        if (diff < 0) diff = -diff;
        if (diff > max) max = diff;
    }
};

template<unsigned N, class Pixel1, class Storage1,
    class Pixel2, class Storage2>
double compute_max_difference(
    image<N,Pixel1,Storage1> const& img1,
    image<N,Pixel2,Storage2> const& img2)
{
    check_matching_units(img1.units, img2.units);
    max_difference_op op;
    op.max = 0;
    reduce_binary_op(img1, img2, op);
    return op.max;
}

}
