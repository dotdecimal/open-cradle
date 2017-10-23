#include <cradle/imaging/geometry.hpp>
#include <cradle/imaging/blend.hpp>
#include <cradle/imaging/view_transforms.hpp>
#include <cradle/math/interpolate.hpp>
#include <cradle/common.hpp>
#include <boost/range/metafunctions.hpp>
#include <boost/range/functions.hpp>
#include <vector>
#include <algorithm>
#include <cradle/imaging/utilities.hpp>

namespace cradle {

namespace impl {

    // Check that all the slices in the given range are consistent.
    template<unsigned N, class T, class SP>
    void check_slice_consistency(
        std::vector<image_slice<N,T,SP> > const& slices)
    {
        image_slice<N,T,SP> const& slice0 = slices.front();
        for (typename std::vector<image_slice<N,T,SP> >::const_iterator
            i = slices.begin(); i != slices.end(); ++i)
        {
            if (i->content.size != slice0.content.size)
            {
                throw cradle::exception(
                    "image dimensions is inconsistent across slices");
            }
            if (!same_spatial_mapping(i->content, slice0.content))
            {
                throw cradle::exception(
                    "spatial mapping is inconsistent across slices");
            }
            if (!same_value_mapping(i->content, slice0.content))
            {
                throw cradle::exception(
                    "value mapping is inconsistent across slices");
            }
            if (i->content.units != slice0.content.units)
            {
                throw cradle::exception(
                    "value units are inconsistent across slices");
            }
            if (i->axis != slice0.axis)
            {
                throw cradle::exception(
                    "slice axis is inconsistent across slices");
            }
        }
    }

    template<unsigned N, class T, class SP>
    struct image_slice_comparison
    {
        bool operator()(image_slice<N,T,SP> const& a,
            image_slice<N,T,SP> const& b) const
        { return a.position < b.position; }
    };

    template<unsigned N, class T, class SP>
    std::vector<image_slice<N,T,SP> >
    sort_slices(std::vector<image_slice<N,T,SP> > const& slices)
    {
        std::vector<image_slice<N,T,SP> > sorted_slices = slices;
        sort(sorted_slices.begin(), sorted_slices.end(),
            image_slice_comparison<N,T,SP>());
        return sorted_slices;
    }

    template<unsigned N, class T, class SP>
    image<N + 1,T,shared>
    merge_sorted_slices(
        std::vector<image_slice<N,T,SP> > const& slices,
        regular_grid<1,double> const& interpolation_grid)
    {
        assert(!slices.empty());
        if (slices.empty())
        {
            throw cradle::exception(
                "merge_slices() called with empty slice list");
        }

        image_slice<N,T,SP> const& slice0 = slices.front();

        unsigned axis = slice0.axis;

        // Compute the dimensions for the merged image.
        vector<N + 1,unsigned> dimensions = unslice(
            slice0.content.size, axis, interpolation_grid.n_points[0]);

        image<N + 1,T,unique> tmp;
        create_image(tmp, dimensions);

        // Set the properties for the merged image...

        // value mapping
        copy_value_mapping(tmp, slice0.content);

        tmp.units = slice0.content.units;

        // spatial mapping
        tmp.origin = unslice(slice0.content.origin, axis,
            interpolation_grid.p0[0] - interpolation_grid.spacing[0] / 2);
        for (unsigned i = 0; i < N; ++i)
        {
            unsigned j = i < axis ? i : i + 1;
            tmp.axes[j] = unslice(slice0.content.axes[i], axis, 0);
        }
        tmp.axes[axis] = unslice(
            uniform_vector<N,double>(0), axis,
            interpolation_grid.spacing[0]);

        // While interpolating, prev_real_slice is the real slice immediately
        // before our interpolation position.  next_real_slice is the slice
        // immediately after.
        typename std::vector<image_slice<N,T,SP> >::const_iterator
            prev_real_slice = slices.begin();
        typename std::vector<image_slice<N,T,SP> >::const_iterator
            next_real_slice = slices.begin();
        double interpolated_position = interpolation_grid.p0[0];

        // Interpolate.
        for (unsigned i = 0; i < interpolation_grid.n_points[0]; ++i)
        {
            // Get a view of the merged slice.
            image_slice<N,T,view> merged_slice =
                sliced_view(cast_storage_type<view>(tmp), axis, i);

            // Update the real slice pointers if necessary.
            while (interpolated_position > next_real_slice->position)
            {
                prev_real_slice = next_real_slice;
                ++next_real_slice;
                if (next_real_slice == slices.end())
                {
                    next_real_slice = prev_real_slice;
                    break;
                }
            }

            // This is true before the first real slice and after the last one.
            if (next_real_slice == prev_real_slice)
            {
                copy_pixels(merged_slice.content, next_real_slice->content);
            }
            // Otherwise, we are in between two real slices, so interpolate.
            else
            {
                double fraction =
                    (next_real_slice->position - interpolated_position) /
                    (next_real_slice->position - prev_real_slice->position);
                // Don't extrapolate!
                fraction = clamp(fraction, 0., 1.);
                raw_blend_images(merged_slice.content,
                    prev_real_slice->content, next_real_slice->content,
                    fraction);
            }

            // Increment the interpolated position.
            interpolated_position += interpolation_grid.spacing[0];
        }

        return share(tmp);
    }
}

template<unsigned N, class T, class SP>
image<N + 1,T,shared>
merge_slices(
    check_in_interface& check_in,
    progress_reporter_interface& progress_reporter,
        std::vector<image_slice<N,T,SP> > const& slices)
{
    impl::check_slice_consistency(slices);

        auto sorted_slices = impl::sort_slices(slices);

        progress_reporter(0.2f);

    auto interpolation_grid =
        compute_interpolation_grid(
            map(
                [](image_slice<N,T,SP> const& slice) -> double
                { return slice.position; },
                sorted_slices),
            4.);

        progress_reporter(0.8f);

    return impl::merge_sorted_slices(sorted_slices, interpolation_grid);
}

template<unsigned N, class T, class SP>
image<N + 1,T,shared>
merge_slices(
    std::vector<image_slice<N,T,SP> > const& slices,
    regular_grid<1,double> const& interpolation_grid)
{
    impl::check_slice_consistency(slices);

    auto sorted_slices = impl::sort_slices(slices);

    return impl::merge_sorted_slices(sorted_slices, interpolation_grid);
}

namespace impl {

    template<unsigned N, class SP>
    struct merge_images_fn
    {
        image<N + 1,variant,shared> merged_image;
        std::vector<image_slice<N,variant,SP> > const* slices;
        template<class Pixel>
        void operator()(Pixel)
        {
            std::size_t n_slices = slices->size();
            std::vector<image_slice<N,Pixel,const_view> > views(n_slices);
            for (std::size_t i = 0; i != n_slices; ++i)
            {
                image_slice<N,variant,SP> const& src = (*slices)[i];
                image_slice<N,Pixel,const_view>& dst = views[i];
                dst.content =
                    cast_image<image<N,Pixel,const_view> >(src.content);
                copy_slice_properties(dst, src);
            }
            merged_image = as_variant(merge_slices(views));
        }
    };
}

template<unsigned N, class SP>
image<N + 1,variant,shared>
merge_slices(
    check_in_interface& check_in,
    progress_reporter_interface& progress_reporter,
    std::vector<image_slice<N,variant,SP> > const& slices)
{
    assert(!slices.empty());
    if (slices.empty())
    {
        throw cradle::exception(
            "merge_slices() called with empty slice list");
    }

    progress_reporter(0.5f);

    impl::merge_images_fn<N,SP> fn;
    fn.slices = &slices;
    dispatch_gray_variant(slices.front().content.pixels.type_info, fn);
    return fn.merged_image;
}

}
