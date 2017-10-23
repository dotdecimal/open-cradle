#ifndef CRADLE_IMAGING_MERGE_SLICES_HPP
#define CRADLE_IMAGING_MERGE_SLICES_HPP

#include <cradle/math/common.hpp>
#include <cradle/imaging/variant.hpp>
#include <cradle/imaging/slicing.hpp>

namespace cradle {

// Given a range of N-dimensional slices, this merges the slices to create an
// (N + 1)-dimensional image.
api(fun disk_cached monitored with(N:2;T:variant;SP:shared) execution_class(cpu.x4))
template<unsigned N, class T, class SP>
image<N + 1,T,shared>
merge_slices(std::vector<image_slice<N,T,SP> > const& slices);

// With this version, you can manually specify the interpolation grid.
template<unsigned N, class T, class SP>
image<N + 1,T,shared>
merge_slices(
    std::vector<image_slice<N,T,SP> > const& slices,
    regular_grid<1,double> const& interpolation_grid);

}

#include <cradle/imaging/merge_slices.ipp>

#endif
