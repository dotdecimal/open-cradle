#include <cradle/imaging/statistics.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cradle/imaging/foreach.hpp>

namespace cradle {

struct discretize_pixel_fn
{
    double a, b;
    template<class Discrete, class Continuous>
    void operator()(Discrete& dst, Continuous& src)
    {
        dst = Discrete(src * a + b);
    }
};

template<unsigned N, class DiscreteT, class ContinuousT, class SourceSP>
void discretize(image<N,DiscreteT,shared>& result,
    image<N,ContinuousT,SourceSP> const& src,
    unsigned result_max)
{
    double src_min, src_max;
    auto min_max = get(image_min_max(src));
    src_min = min_max.min;
    src_max = min_max.max;
    double scale = (src_max - src_min) / result_max;

    discretize_pixel_fn fn;
    fn.a = src.value_mapping.slope / scale;
    fn.b = (src.value_mapping.intercept - src_min) / scale + 0.5;

    image<N,DiscreteT,unique> tmp;
    create_image(tmp, src.size);
    foreach_pixel2(tmp, src, fn);

    copy_spatial_mapping(tmp, src);
    tmp.value_mapping = linear_function<double>(src_min, scale);
    tmp.units = src.units;

    result = share(tmp);
}

template<unsigned N, class DiscreteT, class ContinuousT, class SourceSP>
void discretize(image<N,DiscreteT,shared>& result,
    image<N,ContinuousT,SourceSP> const& src,
    linear_function<double> const& value_mapping)
{
    discretize_pixel_fn fn;
    fn.a = src.value_mapping.slope / value_mapping.slope,
    fn.b = (src.value_mapping.intercept - value_mapping.intercept) /
        value_mapping.slope + 0.5;

    image<N,DiscreteT,unique> tmp;
    create_image(tmp, src.size);
    foreach_pixel2(tmp, src, fn);

    copy_spatial_mapping(tmp, src);
    tmp.value_mapping = value_mapping;
    tmp.units = src.units;

    result = share(tmp);
}

namespace impl {

    template<unsigned N, class DiscreteT>
    struct manual_discretize_functor
    {
        image<N,DiscreteT,shared>* result;
        linear_function<double> value_mapping;
        template<class ContinuousT, class SourceSP>
        void operator()(image<N,ContinuousT,SourceSP> const& src)
        {
            discretize(*result, src, value_mapping);
        }
    };
}

template<unsigned N, class DiscreteT, class SourceSP>
void discretize(image<N,DiscreteT,shared>& result,
    image<N,variant,SourceSP> const& src,
    linear_function<double> const& value_mapping)
{
    impl::manual_discretize_functor<N,DiscreteT> fn;
    fn.result = &result;
    fn.value_mapping = value_mapping;
    apply_fn_to_gray_variant(fn, src);
}

}
