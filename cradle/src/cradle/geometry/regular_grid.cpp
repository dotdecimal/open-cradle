#include <cradle/geometry/regular_grid.hpp>

#include <numeric>

namespace cradle {

double
sum_grid_index_weights(std::vector<weighted_grid_index> const& cells)
{
    return
        std::accumulate(cells.begin(), cells.end(), 0.,
            [](double sum, weighted_grid_index const& w)
            {
                return sum + w.weight;
            });
}

}
