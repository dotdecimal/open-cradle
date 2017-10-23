#include <cradle/imaging/geometry.hpp>
#include <cradle/imaging/iterator.hpp>
#include <cmath>

namespace cradle {

namespace impl {

    template<unsigned N, class T, class SP>
    optional<T>
    compute_raw_image_sample(
        image<N,T,SP> const& img, vector<N,double> const& p)
    {
        vector<N,double> image_p =
            transform_point(inverse(get_spatial_mapping(img)), p);
        vector<N,unsigned> index;
        for (unsigned i = 0; i < N; ++i)
        {
            index[i] = unsigned(std::floor(image_p[i]));
            if (index[i] >= img.size[i])
                return optional<T>();
        }
        return T(get_pixel_ref(img, index));
    }

    template<unsigned N, class T, class SP>
    optional<typename replace_channel_type<T,double>::type>
    compute_image_sample(image<N,T,SP> const& img, vector<N,double> const& p)
    {
        auto sample = compute_raw_image_sample(img, p);
        if (!sample)
        {
            return optional<
                typename replace_channel_type<T,double>::type>();
        }
        return apply_linear_function(img.value_mapping, sample.get());
    }

    template<unsigned N>
    struct point_sampler
    {
        vector<N,double> p;
        optional<double> result;
        template<class Pixel, class SP>
        void operator()(image<N,Pixel,SP> const& img)
        {
            optional<Pixel> x = raw_image_sample(img, p);
            if (x)
                result = double(get(x));
            else
                result = none;
        }
    };

    template<unsigned N, class SP>
    optional<double> compute_raw_image_sample(image<N,variant,SP> const& img,
        vector<N,double> const& p)
    {
        impl::point_sampler<N> sampler;
        sampler.p = p;
        apply_fn_to_gray_variant(sampler, img);
        return sampler.result;
    }
    template<unsigned N, class SP>
    optional<double> compute_image_sample(image<N,variant,SP> const& img,
        vector<N,double> const& p)
    {
        auto sample = compute_raw_image_sample(img, p);
        if (!sample)
            return sample;
        return apply_linear_function(img.value_mapping, sample.get());
    }
}

template<unsigned N, class T, class SP>
optional<T>
raw_image_sample(
    image<N,T,SP> const& img, vector<N,double> const& p)
{
    return impl::compute_raw_image_sample(img, p);
}

template<unsigned N, class T, class SP>
optional<typename replace_channel_type<T,double>::type>
image_sample(image<N,T,SP> const& img, vector<N,double> const& p)
{
    return impl::compute_image_sample(img, p);
}

namespace impl {

    template<unsigned N, class T, class SP, unsigned Axis>
    struct raw_interpolated_sampler
    {
        static optional<typename replace_channel_type<T,double>::type>
        apply(image<N,T,SP> const& img,
            vector<N,double> const& p,
            vector<N,unsigned>& index)
        {
            typedef optional<
                typename replace_channel_type<T,double>::type> return_type;

            double v = p[Axis] - 0.5, floor_v = std::floor(v);

            index[Axis] = unsigned(floor_v);
            if (index[Axis] < img.size[Axis] &&
                index[Axis] + 1 < img.size[Axis])
            {
                double f = v - floor_v;

                return_type sample1 =
                    raw_interpolated_sampler<N,T,SP,Axis - 1>::apply(
                    img, p, index);

                ++index[Axis];

                return_type sample2 =
                    raw_interpolated_sampler<N,T,SP,Axis - 1>::apply(
                    img, p, index);

                if (sample1 && sample2)
                    return sample1.get() * (1 - f) + sample2.get() * f;
            }
            else
            {
                index[Axis] = unsigned(std::floor(p[Axis]));
                if (index[Axis] < img.size[Axis])
                {
                    return raw_interpolated_sampler<N,T,SP,Axis - 1>::apply(
                        img, p, index);
                }
            }

            return return_type();
        }
    };
    template<unsigned N, class T, class SP>
    struct raw_interpolated_sampler<N,T,SP,0>
    {
        static optional<typename replace_channel_type<T,double>::type>
        apply(image<N,T,SP> const& img,
            vector<N,double> const& p,
            vector<N,unsigned>& index)
        {
            typedef optional<
                typename replace_channel_type<T,double>::type> return_type;

            double v = p[0] - 0.5, floor_v = std::floor(v);

            index[0] = unsigned(floor_v);
            if (index[0] < img.size[0] && index[0] + 1 < img.size[0])
            {
                double f = v - floor_v;

                typename replace_channel_type<T,double>::type sample1 =
                    channel_convert<double>(get_pixel_ref(img, index));

                ++index[0];

                typename replace_channel_type<T,double>::type sample2 =
                    channel_convert<double>(get_pixel_ref(img, index));

                return sample1 * (1 - f) + sample2 * f;
            }
            else
            {
                index[0] = unsigned(std::floor(p[0]));
                if (index[0] < img.size[0])
                {
                    return channel_convert<double>(get_pixel_ref(img, index));
                }
            }

            return return_type();
        }
    };

    template<unsigned N, class T, class SP>
    optional<typename replace_channel_type<T,double>::type>
    compute_raw_interpolated_image_sample(
        image<N,T,SP> const& img, vector<N,double> const& p)
    {
        vector<N,double> image_p =
            transform_point(inverse(get_spatial_mapping(img)), p);
        vector<N,unsigned> index;
        return impl::raw_interpolated_sampler<N,T,SP,N - 1>::apply(
            img, image_p, index);
    }

    template<unsigned N, class T, class SP>
    optional<typename replace_channel_type<T,double>::type>
    compute_interpolated_image_sample(
        image<N,T,SP> const& img, vector<N,double> const& p)
    {
        auto sample = compute_raw_interpolated_image_sample(img, p);
        if (!sample)
            return sample;
        return apply_linear_function(img.value_mapping, sample.get());
    }

    template<unsigned N>
    struct interpolated_point_sampler
    {
        vector<N,double> p;
        optional<double> result;
        template<class Pixel, class SP>
        void operator()(image<N,Pixel,SP> const& img)
        { result = compute_raw_interpolated_image_sample(img, p); }
    };

    template<unsigned N, class SP>
    optional<double>
    compute_raw_interpolated_image_sample(
        image<N,variant,SP> const& img, vector<N,double> const& p)
    {
        impl::interpolated_point_sampler<N> sampler;
        sampler.p = p;
        apply_fn_to_gray_variant(sampler, img);
        return sampler.result;
    }
    template<unsigned N, class SP>
    optional<double>
    compute_interpolated_image_sample(
        image<N,variant,SP> const& img, vector<N,double> const& p)
    {
        auto sample = raw_interpolated_image_sample(img, p);
        if (!sample)
            return sample;
        return apply_linear_function(img.value_mapping, sample.get());
    }
}

template<unsigned N, class T, class SP>
optional<typename replace_channel_type<T,double>::type>
raw_interpolated_image_sample(
    image<N,T,SP> const& img, vector<N,double> const& p)
{
    return impl::compute_raw_interpolated_image_sample(img, p);
}

template<unsigned N, class T, class SP>
optional<typename replace_channel_type<T,double>::type>
interpolated_image_sample(
    image<N,T,SP> const& img, vector<N,double> const& p)
{
    return impl::compute_interpolated_image_sample(img, p);
}

namespace impl {

    // stores information about which pixels to sample along each axis
    struct raw_region_sample_axis_info
    {
        // the index of the first pixel to include
        unsigned i_begin;
        // # of pixels to include
        unsigned count;
        // w_first is the weight of the first pixel (if count >= 1)
        // w_last is the weight of the last pixel (if count >= 2)
        // w_mid is the weight of the pixels in the middle (if count >= 3)
        double w_first, w_mid, w_last;
    };

    template<unsigned N, class T, class SP, unsigned Axis>
    struct raw_region_sample
    {
        static void apply(
            typename replace_channel_type<T,double>::type& result,
            image<N,T,SP> const& img,
            raw_region_sample_axis_info const* axis_info,
            vector<N,unsigned>& index,
            double weight)
        {
            raw_region_sample_axis_info const& info = axis_info[Axis];

            index[Axis] = info.i_begin;
            raw_region_sample<N,T,SP,Axis - 1>::apply(result, img,
                axis_info, index, weight * info.w_first);

            unsigned count = info.count - 1;
            while (count > 1)
            {
                ++index[Axis];
                raw_region_sample<N,T,SP,Axis - 1>::apply(result, img,
                    axis_info, index, weight * info.w_mid);
                --count;
            }

            if (count > 0)
            {
                ++index[Axis];
                raw_region_sample<N,T,SP,Axis - 1>::apply(result, img,
                    axis_info, index, weight * info.w_last);
            }
        }
    };
    template<unsigned N, class T, class SP>
    struct raw_region_sample<N,T,SP,0>
    {
        static void apply(
            typename replace_channel_type<T,double>::type& result,
            image<N,T,SP> const& img,
            raw_region_sample_axis_info const* axis_info,
            vector<N,unsigned>& index,
            double weight)
        {
            raw_region_sample_axis_info const& info = axis_info[0];

            index[0] = info.i_begin;
            span_iterator<N,T,SP> i = get_axis_iterator(img, 0, index);

            result += typename replace_channel_type<T,double>::type(*i) *
                weight * info.w_first;
            ++i;

            unsigned count = info.count - 1;
            while (count > 1)
            {
                result += typename replace_channel_type<T,double>::type(*i) *
                    weight * info.w_mid;
                ++i;
                --count;
            }

            if (count > 0)
            {
                result += typename replace_channel_type<T,double>::type(*i) *
                    weight * info.w_last;
                ++i;
            }
        }
    };

    template<unsigned N, class T, class SP>
    optional<typename replace_channel_type<T,double>::type>
    compute_raw_image_sample_over_box(
        image<N,T,SP> const& img, box<N,double> const& region)
    {
        matrix<N+1,N+1,double> inverse_spatial_mapping =
            inverse(get_spatial_mapping(img));

        box<N,double> image_region;
        image_region.corner =
            transform_point(inverse_spatial_mapping, region.corner);
        image_region.size =
            transform_vector(inverse_spatial_mapping, region.size);

        impl::raw_region_sample_axis_info axis_info[N];

        for (unsigned i = 0; i < N; ++i)
        {
            impl::raw_region_sample_axis_info& info = axis_info[i];

            double low = double(std::floor(image_region.corner[i]));
            int i_begin = int(low);
            if (i_begin < 0)
            {
                i_begin = 0;
                info.w_first = 1;
            }
            else
                info.w_first = (low + 1) - double(image_region.corner[i]);

            double high = double(std::ceil(get_high_corner(image_region)[i]));
            int i_end = int(high);
            if (i_end > int(img.size[i]))
            {
                i_end = img.size[i];
                info.w_last = 1;
            }
            else
            {
                info.w_last = double(get_high_corner(image_region)[i]) -
                    (high - 1);
            }

            // region doesn't overlap the image along this axis
            if (i_begin >= i_end)
            {
                return optional<
                    typename replace_channel_type<T,double>::type>();
            }

            info.i_begin = i_begin;
            info.count = i_end - i_begin;

            double total_weight = info.w_first;
            if (info.count > 1)
                total_weight += info.w_last + (info.count - 2);

            double adjustment = 1 / total_weight;
            info.w_first *= adjustment;
            info.w_last *= adjustment;
            info.w_mid = adjustment;
        }

        typename replace_channel_type<T,double>::type result;
        fill_channels(result, 0);

        vector<N,unsigned> index;

        impl::raw_region_sample<N,T,SP,N-1>::apply(
            result, img, axis_info, index, 1);

        return result;
    }

    template<unsigned N, class T, class SP>
    optional<typename replace_channel_type<T,double>::type>
    compute_image_sample_over_box(
        image<N,T,SP> const& img, box<N,double> const& region)
    {
        auto sample = compute_raw_image_sample_over_box(img, region);
        if (!sample)
            return sample;
        return apply_linear_function(img.value_mapping, sample.get());
    }

    template<unsigned N>
    struct region_sampler
    {
        box<N,double> region;
        optional<double> result;
        template<class Pixel, class SP>
        void operator()(image<N,Pixel,SP> const& img)
        { result = compute_raw_image_sample_over_box(img, region); }
    };

    template<unsigned N, class SP>
    optional<double>
    compute_raw_image_sample_over_box(
        image<N,variant,SP> const& img, box<N,double> const& region)
    {
        impl::region_sampler<N> sampler;
        sampler.region = region;
        apply_fn_to_gray_variant(sampler, img);
        return sampler.result;
    }
    //
    template<unsigned N, class SP>
    optional<double>
    compute_image_sample_over_box(
        image<N,variant,SP> const& img, box<N,double> const& region)
    {
        auto sample = compute_raw_image_sample_over_box(img, region);
        if (!sample)
            return sample;
        return apply_linear_function(img.value_mapping, sample.get());
    }
}

template<unsigned N, class T, class SP>
optional<typename replace_channel_type<T,double>::type>
raw_image_sample_over_box(
    image<N,T,SP> const& img, box<N,double> const& region)
{
    return impl::compute_raw_image_sample_over_box(img, region);
}

template<unsigned N, class T, class SP>
optional<typename replace_channel_type<T,double>::type>
image_sample_over_box(
    image<N,T,SP> const& img, box<N,double> const& region)
{
    return impl::compute_image_sample_over_box(img, region);
}

}
