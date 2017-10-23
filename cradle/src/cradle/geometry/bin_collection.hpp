/*
 * Author(s):  Salvadore Gerace <sgerace@dotdecimal.com>
 * Date:       12/12/2013
 *
 * Copyright:
 * This work was developed as a joint effort between .decimal, Inc. and
 * Partners HealthCare under research agreement A213686; as such, it is
 * jointly copyrighted by the participating organizations.
 * (c) 2013 .decimal, Inc. All rights reserved.
 * (c) 2013 Partners HealthCare. All rights reserved.
 */

#ifndef CRADLE_GEOMETRY_BIN_COLLECTION_HPP
#define CRADLE_GEOMETRY_BIN_COLLECTION_HPP

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <vector>
#include <boost/numeric/conversion/cast.hpp>
#include <cradle/common.hpp>
#include <cradle/geometry/common.hpp>


namespace cradle {

// --------------------------------------------------
// Public structures

const double EPSILON = 1.0e-8;

// struct used temporarily while building the optimized bin_collection
template<class ItemType, unsigned int N, class T>
struct bin_collection_item
{
    ItemType item;
    box<N, T> bounds;
    bin_collection_item() {}
    bin_collection_item(
        ItemType const& item, box<N,T> const& bounds) : item(item), bounds(bounds) {}
};

// Represents a collection of geometric items which have been bounded by axis-aligned
// bounding boxes in N-dimensional Euclidean space.
api(struct with(ItemType:unsigned;N:3;T:double))
template<class ItemType, unsigned N, class T>
struct bin_collection
{
    // Bounds of all elements in the collection.
    box<N, T> bounds;
    // Spacing of the bins.
    vector<N,unsigned> grid_size;
    // Array of index shifts used for storing bin items (used to locate first item in each
    // within the item array). (u)
    array<unsigned> offsets;
    // Array of item counts per bin. (u)
    array<unsigned> counts;
    // Array of actual items in the bins. (struct: ItemType)
    array<ItemType> bins;
};


// helper for converting from coords to an index into the flat list
template<unsigned int N>
unsigned
coords_to_index(vector<N,unsigned> const& coords, vector<N,unsigned> const& grid_size)
{
    // calculate the step size for moving through each dimension
    vector<N,unsigned> step_sizes;
    step_sizes[0] = 1;
    for (unsigned dim = 1; dim < N; ++dim)
    {
        step_sizes[dim] = step_sizes[dim - 1] * grid_size[dim - 1];
    }

    return dot(coords, step_sizes);
}

// check whether the given coordinates are in-bounds for the grid size
template<unsigned int N>
bool
coords_in_bounds(vector<N,unsigned> const& coords, vector<N,unsigned> const& grid_size)
{
    for (unsigned dim = 0; dim < N; ++dim)
    {
        if (coords[dim] >= grid_size[dim])
        {
            return false;
        }
    }
    return true;
}

// advances coords to the next coord for iterating through a range in a grid. returns true
// if there was a valid next coord
template<unsigned int N>
bool
go_to_next(
    vector<N,unsigned> & coords,
    vector<N,unsigned> const& coord_minimums,
    vector<N,unsigned> const& coord_maximums)
{
    unsigned i = 0;

    while (true)
    {
        if (coords[i] == coord_maximums[i])
        {
            coords[i] = coord_minimums[i];
            i++;
            if (i == N)
            {
                return false;
            }
        }
        else
        {
            coords[i] += 1;
            return true;
        }
    }
}
// advances backwards to the next coord for iterating through a range in a grid. returns
// true if there was a valid prev coord
template<unsigned int N>
bool
go_to_prev(
    vector<N,unsigned> & coords,
    vector<N,unsigned> const& coord_minimums,
    vector<N,unsigned> const& coord_maximums)
{
    unsigned i = 0;
    while (true)
    {
        if (coords[i] == coord_minimums[i])
        {
            coords[i] = coord_maximums[i];
            ++i;
            if (i == N)
            {
                return false;
            }
        }
        else
        {
            coords[i] -= 1;
            return true;
        }
    }
}

// used during make_bin_collection to place items in their bins
template<typename OutputIterator, class ItemType, unsigned int N, class T>
void fill(
    // iterator to beginning of container to fill
    OutputIterator& output,
    // first row/col/etc. to include in each dimension
    vector<N,unsigned> const& starts,
    // last row/col/etc. to include in each dimension
    vector<N,unsigned> const& ends,
    // how many rows/cols/etc. the bincollection has in each dimensions
    vector<N,unsigned> const& grid_size,
    // item to place in each bin in the given range
    bin_collection_item<ItemType,N,T> const& item )
{
    typedef vector<N,unsigned> Coords;

    // keep coords in valid range
    Coords clamped_starts;
    Coords clamped_ends;
    for (unsigned dim = 0; dim < N; ++dim)
    {
        clamped_starts[dim] = std::max(std::min(starts[dim], grid_size[dim] - 1), 0U);
        clamped_ends[dim]   = std::max(std::min(ends[dim],   grid_size[dim] - 1), 0U);
    }

    // early out on nothing to fill
    if (length2(grid_size) == 0)
    {
        return;
    }

    // step through the bins, adding item to them
    Coords current_bin_coords = clamped_starts;
    do
    {
        unsigned i = coords_to_index(current_bin_coords, grid_size);
        (output + i)->push_back(item);
    } while (go_to_next(current_bin_coords, clamped_starts, clamped_ends));
}

// convert a filled grid of bin_items to the optimized bin_collection struct
template<typename InputIterator, class ItemType, unsigned int N, class T>
bin_collection<ItemType, N, T>
optimize_to_bin_collection(
    InputIterator const& in_bins,
    vector<N,unsigned> grid_size,
    box<N, T> bounds)
{
    typedef bin_collection_item<ItemType, N, T> BinItem;
    typedef std::vector<BinItem> ItemList;

    bin_collection<ItemType, N, T> output;
    output.bounds = bounds;
    output.grid_size = grid_size;

    unsigned total_bins = 1;
    for (unsigned dim = 0; dim < N; ++dim)
    {
        total_bins *= grid_size[dim];
    }

    unsigned* offsets = allocate(&output.offsets, total_bins);
    unsigned* counts = allocate(&output.counts, total_bins);

    // figure out how long bin_array is and fill counts array
    unsigned bin_array_length = 0;
    for (unsigned i = 0; i < total_bins; ++i)
    {
        unsigned count = boost::numeric_cast<unsigned>((in_bins + i)->size());
        counts[i] = count;
        bin_array_length += count;
    }

    ItemType* bins = allocate(&output.bins, bin_array_length);

    // copy values into flattened bin array
    unsigned cur_offset = 0;
    for (unsigned i = 0; i < total_bins; ++i)
    {
        ItemList const& bin = *(in_bins + i);
        offsets[i] = cur_offset;
        for (auto iter = bin.begin(); iter != bin.end(); ++iter)
        {
            bins[cur_offset] = iter->item;
            ++cur_offset;
        }
    }

    return output;
}

template <typename InputIterator, typename GetBoundsFnType,
    class ItemType, unsigned int N, class T>
bin_collection<ItemType, N, T>
make_bin_collection(
    InputIterator const& in_iter_begin,
    InputIterator const& in_iter_end,
    GetBoundsFnType get_bounds_fn,
    unsigned resolution_multiplier = 2)
{
    typedef bin_collection_item<ItemType, N, T> BinItem;
    typedef std::vector<BinItem> ItemList;
    typedef std::vector<ItemList> ItemListList;
    typedef vector<N,unsigned> Coords;
    typedef vector<N,T> Vector;

    Vector bounds_min;
    Vector bounds_max;
    Vector size_sums;
    for (unsigned dim = 0; dim < N; ++dim)
    {
        bounds_min[dim] = (std::numeric_limits<T>::max)();
        bounds_max[dim] = std::numeric_limits<T>::lowest();
        size_sums[dim] = 0;
    }

    // create bin_items, keep track of bounds of things
    std::map<ItemType, BinItem> bin_items;
    unsigned num_items = 0;
    for (auto iter = in_iter_begin; iter != in_iter_end; ++iter)
    {
        ItemType const& val = *iter;
        bin_items[val] = BinItem(val, get_bounds_fn(val));
        auto const& b = bin_items[val].bounds;
        for (unsigned dim = 0; dim < N; ++dim)
        {
            if (b.corner[dim] < bounds_min[dim])
            {
                bounds_min[dim] = b.corner[dim];
            }
            if ((b.corner[dim] + b.size[dim]) > bounds_max[dim])
            {
                bounds_max[dim] = b.corner[dim] + b.size[dim];
            }
        }
        ++num_items;
        size_sums += b.size;
    }

    // make the big bins list
    Coords grid_size;
    box3d bounds(bounds_min, bounds_max - bounds_min);
    for (unsigned dim = 0; dim < N; ++dim)
    {
        grid_size[dim] =
            unsigned(size_sums[dim] > EPSILON ?
                std::ceil(double(resolution_multiplier) * double(num_items) *
                    bounds.size[dim] / size_sums[dim]) : 1);
    }
    ItemListList bins;
    unsigned num_bins = 1;
    for (unsigned dim = 0; dim < N; ++dim)
    {
        num_bins *= grid_size[dim];
    }
    bins.resize(num_bins);

    // add items to the bins
    for (auto iter = bin_items.begin(); iter != bin_items.end(); ++iter)
    {
        // start in each row or column or (...etc into more dimensions) of the range that
        // includes the item
        Coords starts;
        // end in each row or column or (...etc into more dimensions) of the range that
        // includes the item
        Coords ends;

        for (unsigned dim = 0; dim < N; ++dim)
        {
            auto& item_bounds = iter->second.bounds;
            auto item_min = item_bounds.corner[dim];
            auto item_max = item_bounds.corner[dim] + item_bounds.size[dim];
            auto mesh_min = bounds.corner[dim];
            auto mesh_max = bounds.corner[dim] + bounds.size[dim];

            starts[dim] =
                unsigned(grid_size[dim] * (item_min - mesh_min) / bounds.size[dim]);
            ends[dim] =
                unsigned(grid_size[dim] * (item_max - mesh_min) / bounds.size[dim]);
        }

        typename ItemListList::iterator bins_begin = bins.begin();
        cradle::fill(bins_begin, starts, ends, grid_size, iter->second);
    }

    return
        optimize_to_bin_collection<typename ItemListList::const_iterator, ItemType, N, T>(
            bins.begin(),
            grid_size,
            bounds);
}

template<class ItemType, unsigned int N, class T>
vector<N, unsigned>
get_coords_for_point(
    bin_collection<ItemType, N, T> const& bin_col,
    vector<N, T> const& point)
{
    typedef vector<N,T> Vector;
    typedef vector<N,unsigned> Coords;

    Vector point_as_offset_from_corner = point - bin_col.bounds.corner;
    Coords coords;
    for (unsigned dim = 0; dim < N; ++dim)
    {
        coords[dim] =
            unsigned(
                double(bin_col.grid_size[dim]) *
                (point[dim] - bin_col.bounds.corner[dim]) / bin_col.bounds.size[dim]);

        coords[dim] = cradle::clamp(coords[dim], 0U, bin_col.grid_size[dim]);
    }
    return coords;
}

// given the grid coordinates, get an iterator to the first item in that bin
template<class ItemType, unsigned int N, class T>
ItemType const*
get_bin_begin(
    bin_collection<ItemType, N, T> const& bin_col,
    vector<N, unsigned> const& coords)
{
    unsigned i = coords_to_index(coords, bin_col.grid_size);
    unsigned offset = bin_col.offsets[i];
    return bin_col.bins.begin() + offset;
}

// given the grid coordinates, get an iterator (just a pointer in this case) one past the
// last item in that bin
template<class ItemType, unsigned int N, class T>
ItemType const*
get_bin_end(
    bin_collection<ItemType, N, T> const& bin_col,
    vector<N, unsigned> const& coords)
{
    unsigned i = coords_to_index(coords, bin_col.grid_size);
    unsigned offset = bin_col.offsets[i];
    unsigned count = bin_col.counts[i];
    return bin_col.bins.begin() + offset + count;
}

// step over the bin collection
// starting in the bin containing {start_point}
// move along axis {axis}
// in the positive direction or negative direction based on {positive_direction}
// calling {fn} on every item found in those bins
// adding up their results and storing it in {*sum} if not null
// if any of those calls to {fn} return false,
// this function returns false instead of setting {*sum}.
//
// fn will be called with the following args:
// (ItemType const& item, SumType*)
// returning whether it was successful (true)
// and if successful, should set the value {*value} if not null.
template<typename FunctionType, typename SumType, class ItemType, unsigned int N, class T>
bool
do_sum(
    bin_collection<ItemType, N, T> const& bin_col,
    vector<N, T> const& start_point,
    unsigned axis,
    bool positive_direction,
    FunctionType fn,
    SumType* sum )
{
    typedef vector<N,unsigned> Coords;

    SumType tempsum = 0;

    // early out if start point is not even within the bin collection bounds
    if (!contains(bin_col.bounds, start_point))
    {
        return false;
    }

    Coords coords = get_coords_for_point(bin_col, start_point);
    Coords min = coords;
    Coords max = coords;
    if (positive_direction)
    {
        max[axis] = bin_col.grid_size[axis] - 1;
    }
    else
    {
        min[axis] = 0;
    }

    std::set<ItemType> items_checked;

    bool keep_going = coords_in_bounds(coords, bin_col.grid_size);
    while (keep_going)
    {
        SumType temp_result;

        auto end = get_bin_end(bin_col, coords);
        for (auto iter = get_bin_begin(bin_col, coords); iter != end; ++iter)
        {
            if (items_checked.find(*iter) == items_checked.end())
            {
                items_checked.insert(*iter);
                bool success = fn(*iter, &temp_result);
                if (!success)
                {
                    return false;
                }
                tempsum += temp_result;
            }
        }

        if (positive_direction)
        {
            keep_going = go_to_next(coords, min, max);
        }
        else
        {
            keep_going = go_to_prev(coords, min, max);
        }
    }

    *sum = tempsum;
    return true;
}

}

#endif