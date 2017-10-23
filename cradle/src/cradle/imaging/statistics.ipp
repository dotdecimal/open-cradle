#include <utility>
#include <cradle/imaging/channel.hpp>
#include <cradle/imaging/foreach.hpp>
#include <cradle/imaging/geometry.hpp>

namespace cradle {

namespace impl {

    template<class T>
    struct min_max_pixel_fn
    {
        T min, max;
        void operator()(T const& v)
        {
            if (v < min)
                min = v;
            if (v > max)
                max = v;
        }
    };
}

template<unsigned N, class T, class SP>
optional<min_max<T> >
raw_image_min_max(image<N,T,SP> const& img)
{
    if (product(img.size) != 0)
    {
        impl::min_max_pixel_fn<T> fn;
        fn.min = fn.max = *get_iterator(img.pixels);
        foreach_pixel(img, fn);
        return min_max<T>(fn.min, fn.max);
    }
    else
        return none;
}

namespace impl {

    template<unsigned N, class T, class SP>
    optional<min_max<double> >
    compute_image_min_max(image<N,T,SP> const& img)
    {
        auto raw_min_max = raw_image_min_max(img);
        if (raw_min_max)
        {
            return min_max<double>(
                apply(img.value_mapping, get(raw_min_max).min),
                apply(img.value_mapping, get(raw_min_max).max));
        }
        else
            return none;
    }

    template<unsigned N>
    struct image_min_max_fn
    {
        optional<min_max<double> > result;
        template<class Pixel,class SP>
        void operator()(image<N,Pixel,SP> const& img)
        { result = impl::compute_image_min_max(img); }
    };

    template<unsigned N, class SP>
    optional<min_max<double> >
    compute_image_min_max(image<N,variant,SP> const& img)
    {
        impl::image_min_max_fn<N> fn;
        apply_fn_to_gray_variant(fn, img);
        return fn.result;
    }
}

template<unsigned N, class T, class SP>
optional<min_max<double> >
image_min_max(image<N,T,SP> const& img)
{
    return impl::compute_image_min_max(img);
}

template<class T>
optional<min_max<T> >
merge_min_max_values(std::vector<optional<min_max<T> > > const& values)
{
    optional<min_max<double> > merged;
    for (auto const& i : values)
    {
        if (i)
        {
            if (merged)
            {
                merged = min_max<T>(
                    std::min(get(i).min, get(merged).min),
                    std::max(get(i).max, get(merged).max));
            }
            else
                merged = i;
        }
    }
    return merged;
}

template<unsigned N, class T, class SP>
optional<min_max<double> >
image_list_min_max(std::vector<image<N,T,SP> > const& imgs)
{
    return merge_min_max_values(map(
        [](image<N,T,SP> const& img) { return image_min_max(img); },
        imgs));
}

namespace impl {

    template<class T>
    struct statistics_pixel_fn
    {
        T min, max;
        size_t max_element_index;
        size_t current_elemenet_index;
        typename sum_type<T>::type sum;
        void operator()(T const& v)
        {
            if (v < min)
            {
                min = v;
            }
            if (v > max)
            {
                max = v;
                max_element_index = current_elemenet_index;
            }
            sum += v;
            ++current_elemenet_index;
        }
    };
}

template<unsigned N, class T, class SP>
statistics<T> raw_image_statistics(image<N,T,SP> const& img)
{
    size_t n_samples = product(img.size);
    if (n_samples != 0)
    {
        impl::statistics_pixel_fn<T> fn;
        fn.min = fn.max = *get_iterator(img.pixels);
        fn.sum = 0;
        fn.max_element_index = fn.current_elemenet_index = 0;
        foreach_pixel(img, fn);
        return statistics<T>(
            fn.min,
            fn.max,
            T(fn.sum / n_samples),
            double(n_samples),
            fn.max_element_index);
    }
    else
        return statistics<T>(none, none, none, 0, none);
}

namespace impl {

    template<unsigned N, class T, class SP>
    statistics<double> compute_image_statistics(image<N,T,SP> const& img)
    {
        size_t n_samples = product(img.size);
        if (n_samples != 0)
        {
            impl::statistics_pixel_fn<T> fn;
            fn.min = fn.max = *get_iterator(img.pixels);
            fn.sum = 0;
            fn.max_element_index = fn.current_elemenet_index = 0;
            foreach_pixel(img, fn);
            return statistics<double>(
                apply(img.value_mapping, fn.min),
                apply(img.value_mapping, fn.max),
                apply(img.value_mapping, double(fn.sum) / n_samples),
                double(n_samples),
                fn.max_element_index);
        }
        else
            return statistics<double>(none, none, none, 0, none);
    }

    template<unsigned N>
    struct image_statistics_fn
    {
        statistics<double> result;
        template<class Pixel,class SP>
        void operator()(image<N,Pixel,SP> const& img)
        { result = impl::compute_image_statistics(img); }
    };

    template<unsigned N, class SP>
    statistics<double> compute_image_statistics(image<N,variant,SP> const& img)
    {
        impl::image_statistics_fn<N> fn;
        apply_fn_to_gray_variant(fn, img);
        return fn.result;
    }
}

template<unsigned N, class T, class SP>
statistics<double> image_statistics(image<N,T,SP> const& img)
{
    return impl::compute_image_statistics(img);
}

namespace impl {

    template<unsigned N, class T, class SP>
    statistics<double>
    compute_partial_image_statistics(
        image<N,T,SP> const& img, std::vector<size_t> const& indices)
    {
        if (!indices.empty())
        {
            typename image<N,T,SP>::iterator_type pixels =
                get_iterator(img.pixels);
            size_t index_limit = product(img.size);
            optional<T> img_min, img_max;
            typename sum_type<T>::type sum = 0;
            size_t n_samples = 0;
            size_t max_element_index;
            for (auto index : indices)
            {
                // TODO: See if this check affects performance.
                if (index >= index_limit)
                    throw exception("image index out of range");
                T x = *(pixels + index);
                if (!img_min || x < get(img_min))
                {
                    img_min = x;
                }
                if (!img_max || x > get(img_max))
                {
                    img_max = x;
                    max_element_index = index;
                }
                sum += x;
                ++n_samples;
            }
            return statistics<double>(
                img_min ?
                    optional<double>(apply(img.value_mapping, get(img_min))) :
                    none,
                img_max ?
                    optional<double>(apply(img.value_mapping, get(img_max))) :
                    none,
                apply(img.value_mapping, double(sum) / double(n_samples)),
                double(n_samples),
                max_element_index);
        }
        else
            return statistics<double>(none, none, none, 0, none);
    }

    template<unsigned N>
    struct partial_image_statistics_fn
    {
        statistics<double> result;
        std::vector<size_t> const* indices;
        template<class Pixel,class SP>
        void operator()(image<N,Pixel,SP> const& img)
        { result = impl::compute_partial_image_statistics(img, *indices); }
    };

    template<unsigned N, class SP>
    statistics<double>
    compute_partial_image_statistics(
        image<N,variant,SP> const& img, std::vector<size_t> const& indices)
    {
        impl::partial_image_statistics_fn<N> fn;
        fn.indices = &indices;
        apply_fn_to_gray_variant(fn, img);
        return fn.result;
    }
}

template<unsigned N, class T, class SP>
statistics<double>
partial_image_statistics(
    image<N,T,SP> const& img, std::vector<size_t> const& indices)
{
    return impl::compute_partial_image_statistics(img, indices);
}

namespace impl {

    template<unsigned N, class T, class SP>
    statistics<double>
    compute_weighted_partial_image_statistics(
        image<N,T,SP> const& img,
        std::vector<weighted_grid_index> const& indices)
    {
        if (!indices.empty())
        {
            typename image<N,T,SP>::iterator_type pixels =
                get_iterator(img.pixels);
            size_t index_limit = product(img.size);
            optional<double> img_min, img_max;
            double sum = 0;
            double n_samples = 0;
            size_t max_element_index;
            for (auto const& index : indices)
            {
                if (index.weight < 1.0e-8)
                {
                    continue;
                }

                // TODO: See if this check affects performance.
                if (index.index >= index_limit)
                {
                    throw exception("image index out of range");
                }

                double x = double(apply(img.value_mapping, *(pixels + index.index)));
                if (!img_min || x < get(img_min))
                {
                    img_min = x;
                }
                if (!img_max || x > get(img_max))
                {
                    img_max = x;
                    max_element_index = index.index;
                }
                sum += x * index.weight;
                n_samples += index.weight;
            }
            return statistics<double>(
                img_min,
                img_max,
                optional<double>(sum / n_samples),
                n_samples,
                max_element_index);
        }
        else
            return statistics<double>(none, none, none, 0, none);
    }

    template<unsigned N>
    struct weighted_partial_image_statistics_fn
    {
        statistics<double> result;
        std::vector<weighted_grid_index> const* indices;
        template<class Pixel,class SP>
        void operator()(image<N,Pixel,SP> const& img)
        {
            result =
                impl::compute_weighted_partial_image_statistics(img, *indices);
        }
    };

    template<unsigned N, class SP>
    statistics<double>
    compute_weighted_partial_image_statistics(
        image<N,variant,SP> const& img,
        std::vector<weighted_grid_index> const& indices)
    {
        impl::weighted_partial_image_statistics_fn<N> fn;
        fn.indices = &indices;
        apply_fn_to_gray_variant(fn, img);
        return fn.result;
    }
}

template<unsigned N, class T, class SP>
statistics<double>
weighted_partial_image_statistics(
    image<N,T,SP> const& img,
    std::vector<weighted_grid_index> const& indices)
{
    return impl::compute_weighted_partial_image_statistics(img, indices);
}

template<class T>
statistics<T>
merge_statistics(std::vector<statistics<T> > const& stats)
{
    double sum = 0;
    optional<T> merged_min, merged_max;
    double n_samples = 0;
    optional<size_t> max_element_index;
    for (auto const& i : stats)
    {
        if (!merged_min || i.min && get(i.min) < get(merged_min))
        {
            merged_min = i.min;
        }
        if (!merged_max || i.max && get(i.max) > get(merged_max))
        {
            merged_max = i.max;
            max_element_index = i.max_element_index;
        }
        n_samples += i.n_samples;
        if (i.mean)
        {
            sum += get(i.mean) * i.n_samples;
        }
    }
    return statistics<T>(
        merged_min, merged_max,
        n_samples > 0 ? optional<T>(T(sum / n_samples)) : none,
        n_samples,
        max_element_index);
}

template<unsigned N, class T, class SP>
statistics<double>
image_list_statistics(std::vector<image<N,T,SP> > const& imgs)
{
    return merge_statistics(map(
        [](image<N,T,SP> const& img) { return image_statistics(img); },
        imgs));
}

namespace impl {

    template<class Bin>
    optional<double>
    variance_from_image_stats(
        image<1,Bin,shared> const& histogram,
        statistics<double> const& stats)
    {
        if (!stats.mean)
            return none;

        auto spatial_mapping = get_spatial_mapping(histogram);

        double total_variance = 0, total_weight = 0;
        for (unsigned i = 0; i < histogram.size[0]; ++i)
        {
            Bin weight = histogram.pixels.view[i];
            if (weight != 0)
            {
                total_variance +=
                    double(weight) * square(transform_point(spatial_mapping,
                        make_vector<double>(i))[0] - get(stats.mean));
                total_weight += double(weight);
            }
        }

        return total_variance / total_weight;
    }

    template<class Bin>
    optional<double>
    standard_deviation_from_image_stats(
        image<1,Bin,shared> const& histogram,
        statistics<double> const& stats)
    {
        auto variance = variance_from_image_stats(histogram, stats);
        return variance ? some(sqrt(get(variance))) : none;
    }

    struct standard_deviation_from_image_stats_fn
    {
        optional<double> result;
        statistics<double> const* stats;
        template<class Pixel>
        void operator()(image<1,Pixel,shared> const& histogram)
        {
            this->result =
                impl::standard_deviation_from_image_stats(histogram, *stats);
        }
    };

    optional<double> static
    standard_deviation_from_image_stats(
        image1 const& histogram,
        statistics<double> const& stats)
    {
        standard_deviation_from_image_stats_fn fn;
        fn.stats = &stats;
        apply_fn_to_gray_variant(fn, histogram);
        return fn.result;
    }
}

template<class Bin>
optional<double>
standard_deviation_from_image_stats(
    image<1,Bin,shared> const& histogram,
    statistics<double> const& stats)
{
    return impl::standard_deviation_from_image_stats(histogram, stats);
}

}
