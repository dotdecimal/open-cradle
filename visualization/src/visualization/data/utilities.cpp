#include <visualization/data/utilities.hpp>

namespace visualization {

optional<double>
array_max(std::vector<double> const& numbers)
{
    optional<double> max;
    for (auto const& n : numbers)
    {
        if (!max || get(max) < n)
            max = n;
    }
    return max;
}

}
