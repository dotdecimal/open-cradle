#include <cradle/imaging/histogram.hpp>
#include <limits>
#include <cradle/imaging/contiguous.hpp>
#include <cradle/imaging/utilities.hpp>

namespace cradle {

template<class Bin, unsigned N, class Pixel, class Storage>
image<1,Bin,shared>
compute_histogram(image<N,Pixel,Storage> const& img,
    double min_value, double max_value, double bin_size)
{
    int max_bin = int((max_value - min_value) / bin_size);
    int n_bins = max_bin + 1;
    image<1,Bin,unique> bin_img;
    create_image_for_histogram(bin_img, n_bins, min_value, bin_size);
    accumulate_histogram(bin_img.pixels.ptr, n_bins, min_value, bin_size, img);
    return share(bin_img);
}

template<class Bin>
void create_image_for_histogram(image<1,Bin,unique>& bin_img,
    size_t n_bins, double min_value, double bin_size)
{
    create_image(bin_img, make_vector<unsigned>(unsigned(n_bins)));
    fill_pixels(bin_img, Bin(0));
    set_spatial_mapping(bin_img, make_vector(min_value),
        make_vector(bin_size));
}

template<class Bin>
struct accumulate_histogram_pixel_fn
{
    Bin* bins;
    size_t n_bins;
    linear_function<double> value_mapping;
    template<class Pixel>
    void operator()(Pixel const& p)
    {
        size_t bin = size_t(std::floor(apply(value_mapping, p)));
        if (bin >= n_bins)
            return; //throw exception("histogram bin out of range");
        ++bins[bin];
    }
};

template<class Bin, unsigned N, class Pixel, class Storage>
void accumulate_histogram(Bin* bins, size_t n_bins,
    double min_value, double bin_size, image<N,Pixel,Storage> const& img)
{
    accumulate_histogram_pixel_fn<Bin> fn;

    fn.bins = bins;

    fn.n_bins = n_bins;

    fn.value_mapping = img.value_mapping;
    fn.value_mapping.intercept -= min_value;
    fn.value_mapping.intercept /= bin_size;
    fn.value_mapping.slope /= bin_size;

    foreach_pixel(img, fn);
}

template<class Bin>
struct accumulate_histogram_fn
{
    Bin* bins;
    size_t n_bins;
    double min_value;
    double bin_size;
    template<unsigned N, class Pixel, class SP>
    void operator()(image<N,Pixel,SP> const& img)
    { accumulate_histogram(bins, n_bins, min_value, bin_size, img); }
};

template<class Bin, unsigned N, class Storage>
void accumulate_histogram(Bin* bins, size_t n_bins,
    double min_value, double bin_size, image<N,variant,Storage> const& img)
{
    accumulate_histogram_fn<Bin> fn;
    fn.bins = bins;
    fn.n_bins = n_bins;
    fn.min_value = min_value;
    fn.bin_size = bin_size;
    apply_fn_to_gray_variant(fn, img);
}

// PARTIAL HISTOGRAM

template<class Bin, class PixelIterator>
void accumulate_partial_histogram(
    Bin* bins, size_t n_bins,
    double min_value, double bin_size,
    PixelIterator pixels, size_t n_pixels,
    linear_function<double> const& value_mapping,
    size_t const* indices, size_t n_indices)
{
    linear_function<double> mapping = value_mapping;
    mapping.intercept -= min_value;
    mapping.intercept /= bin_size;
    mapping.slope /= bin_size;

    size_t const* end_i = indices + n_indices;
    for (size_t const* i = indices; i != end_i; ++i)
    {
        size_t index = *i;
        // TODO: See if these checks affect performance.
        if (index >= n_pixels)
            throw exception("image index out of range");
        size_t bin =
            size_t(std::floor(apply(mapping, *(pixels + index))));
        if (bin >= n_bins)
            continue; //throw exception("histogram bin out of range");
        ++bins[bin];
    }
}

template<class Bin, unsigned N, class Pixel, class Storage>
void accumulate_partial_histogram(
    Bin* bins, size_t n_bins,
    double min_value, double bin_size,
    image<N,Pixel,Storage> const& img,
    size_t const* indices, size_t n_indices)
{
    assert(is_contiguous(img));
    accumulate_partial_histogram(bins, n_bins, min_value, bin_size,
        get_iterator(img.pixels), product(img.size), img.value_mapping,
        indices, n_indices);
}

template<class Bin>
struct accumulate_partial_histogram_fn
{
    Bin* bins;
    size_t n_bins;
    double min_value;
    double bin_size;
    size_t const* indices;
    size_t n_indices;
    template<unsigned N, class Pixel, class SP>
    void operator()(image<N,Pixel,SP> const& img)
    { accumulate_partial_histogram(bins, n_bins, min_value, bin_size, img,
        indices, n_indices); }
};

template<class Bin, unsigned N, class Storage>
void accumulate_partial_histogram(Bin* bins, size_t n_bins,
    double min_value, double bin_size,
    image<N,variant,Storage> const& img,
    size_t const* indices, size_t n_indices)
{
    accumulate_partial_histogram_fn<Bin> fn;
    fn.bins = bins;
    fn.n_bins = n_bins;
    fn.min_value = min_value;
    fn.bin_size = bin_size;
    fn.indices = indices;
    fn.n_indices = n_indices;
    apply_fn_to_gray_variant(fn, img);
}

template<class Bin, unsigned N, class Pixel, class Storage>
image<1,Bin,shared>
compute_partial_histogram(
    image<N,Pixel,Storage> const& img,
    size_t const* indices, size_t n_indices,
    double min_value, double max_value, double bin_size)
{
    int max_bin = int((max_value - min_value) / bin_size);
    int n_bins = max_bin + 1;
    image<1,Bin,unique> bin_img;
    create_image_for_histogram(bin_img, n_bins, min_value, bin_size);
    accumulate_partial_histogram(bin_img.pixels.ptr, n_bins, min_value,
        bin_size, img, indices, n_indices);
    return share(bin_img);
}

// WEIGHTED PARATIAL HISTOGRAM

template<class Bin, class PixelIterator>
void accumulate_partial_histogram(
    Bin* bins, size_t n_bins,
    double min_value, double bin_size,
    PixelIterator pixels, size_t n_pixels,
    linear_function<double> const& value_mapping,
    weighted_grid_index const* indices, size_t n_indices)
{
    linear_function<double> mapping = value_mapping;
    mapping.intercept -= min_value;
    mapping.intercept /= bin_size;
    mapping.slope /= bin_size;

    weighted_grid_index const* end_i = indices + n_indices;
    for (weighted_grid_index const* i = indices; i != end_i; ++i)
    {
        // TODO: See if these checks affect performance.
        if (i->index >= n_pixels)
            throw exception("image index out of range");
        size_t bin =
            size_t(std::floor(apply(mapping, *(pixels + i->index))));
        if (bin >= n_bins)
            continue; //throw exception("histogram bin out of range");
        bins[bin] += i->weight;
    }
}

template<class Bin, unsigned N, class Pixel, class Storage>
void accumulate_partial_histogram(Bin* bins, size_t n_bins,
    double min_value, double bin_size,
    image<N,Pixel,Storage> const& img,
    weighted_grid_index const* indices, size_t n_indices)
{
    assert(is_contiguous(img));
    accumulate_partial_histogram(bins, n_bins, min_value, bin_size,
        get_iterator(img.pixels), product(img.size), img.value_mapping,
        indices, n_indices);
}

template<class Bin>
struct accumulate_weighted_partial_histogram_fn
{
    Bin* bins;
    size_t n_bins;
    double min_value;
    double bin_size;
    weighted_grid_index const* indices;
    size_t n_indices;
    template<unsigned N, class Pixel, class SP>
    void operator()(image<N,Pixel,SP> const& img)
    { accumulate_partial_histogram(bins, n_bins, min_value, bin_size, img,
        indices, n_indices); }
};

template<class Bin, unsigned N, class Storage>
void accumulate_partial_histogram(Bin* bins, size_t n_bins,
    double min_value, double bin_size,
    image<N,variant,Storage> const& img,
    weighted_grid_index const* indices, size_t n_indices)
{
    accumulate_weighted_partial_histogram_fn<Bin> fn;
    fn.bins = bins;
    fn.n_bins = n_bins;
    fn.min_value = min_value;
    fn.bin_size = bin_size;
    fn.indices = indices;
    fn.n_indices = n_indices;
    apply_fn_to_gray_variant(fn, img);
}

template<class Bin, unsigned N, class Pixel, class Storage>
image<1,Bin,shared>
compute_partial_histogram(
    image<N,Pixel,Storage> const& img,
    weighted_grid_index const* indices, size_t n_indices,
    double min_value, double max_value, double bin_size)
{
    int max_bin = int((max_value - min_value) / bin_size);
    int n_bins = max_bin + 1;
    image<1,Bin,unique> bin_img;
    create_image_for_histogram(bin_img, n_bins, min_value, bin_size);
    accumulate_partial_histogram(bin_img.pixels.ptr, n_bins, min_value,
        bin_size, img, indices, n_indices);
    return share(bin_img);
}

// RAW HISTOGRAM

struct accumulate_raw_histogram_pixel_fn
{
    unsigned* hist;
    template<class Pixel>
    void operator()(Pixel const& p)
    {
        typedef typename unsigned_channel_type<Pixel>::type unsigned_t;
        ++hist[unsigned_t(p)];
    }
};

template<unsigned N, class Pixel, class Storage>
void accumulate_raw_histogram(unsigned* hist,
    image<N,Pixel,Storage> const& img)
{
    accumulate_raw_histogram_pixel_fn fn;
    fn.hist = hist;
    foreach_pixel(img, fn);
}

template<unsigned N, class Pixel, class Storage>
void compute_raw_histogram(std::vector<unsigned>* hist,
    image<N,Pixel,Storage> const& img)
{
    typedef typename unsigned_channel_type<Pixel>::type unsigned_t;
    hist->resize(unsigned((std::numeric_limits<unsigned_t>::max)()) + 1, 0);
    accumulate_raw_histogram(&(*hist)[0], img);
}

}
