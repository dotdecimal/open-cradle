#ifndef VISUALIZATION_DATA_UTILITIES_HPP
#define VISUALIZATION_DATA_UTILITIES_HPP

#include <cradle/geometry/regular_grid.hpp>

#include <visualization/common.hpp>

namespace visualization {

// Get the maximum value within an array of numbers.
api(fun)
optional<double>
array_max(std::vector<double> const& numbers);

}

#endif
