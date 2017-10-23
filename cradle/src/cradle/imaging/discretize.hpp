#ifndef CRADLE_IMAGING_DISCRETIZE_HPP
#define CRADLE_IMAGING_DISCRETIZE_HPP

#include <cradle/imaging/variant.hpp>

namespace cradle {

template<unsigned N, class DiscreteT, class ContinuousT, class SourceSP>
void discretize(image<N,DiscreteT,shared>& result,
    image<N,ContinuousT,SourceSP> const& src,
    unsigned result_max);

template<unsigned N, class DiscreteT, class ContinuousT, class SourceSP>
void discretize(image<N,DiscreteT,shared>& result,
    image<N,ContinuousT,SourceSP> const& src,
    linear_function<double> const& value_mapping);

template<unsigned N, class DiscreteT, class SourceSP>
void discretize(image<N,DiscreteT,shared>& result,
    image<N,variant,SourceSP> const& src,
    linear_function<double> const& value_mapping);

}

#include <cradle/imaging/discretize.ipp>

#endif
