#include <cradle/geometry/slicing.hpp>

namespace cradle {

bool is_inside(slice_description const& s, double p)
{
    return p >= s.position - s.thickness / 2 &&
        p < s.position + s.thickness / 2;
}

// Note this function relies on the image slice descriptions being sorted by
// position low to high.
bool is_inside(slice_description_list const& slices, double p)
{
    if (slices.size() == 0 ||
        p < slices[0].position - 0.5 * slices[0].thickness ||
        p > slices.back().position + 0.5 * slices.back().thickness)
    {
        return false;
    }
    else
    {
        return true;
    }
}

size_t
find_image_slice_index(slice_description_list const& slices, double p)
{
    if (slices.size() == 0 ||
        p < slices[0].position - 0.5 * slices[0].thickness ||
        p > slices.back().position + 0.5 * slices.back().thickness)
    {
        return slices.size();
    }

    for (size_t i = 1; i < slices.size(); ++i)
    {
        if (p < slices[i].position)
        {
            return
                (p - slices[i-1].position < slices[i].position - p) ?
                i - 1 : i;
        }
    }
    return slices.size() - 1;
}

double round_slice_position(slice_description_list const& slices, double p)
{
    if (slices.empty())
        throw exception("empty slice list");
    for (auto const& i : slices)
    {
        if (p < i.position + i.thickness / 2)
            return i.position;
    }
    return slices.back().position;
}

double advance_slice_position(
    slice_description_list const& slices, double p, int n)
{
    if (slices.empty())
        throw exception("can't advance through empty slice list");
    auto index = int(find_image_slice_index(slices, p));
    int advanced_index = clamp(index + n, 0, int(slices.size()) - 1);
    return slices[advanced_index].position;
}

box1d get_slice_list_bounds(slice_description_list const& slices)
{
    if (slices.empty())
        throw exception("bounds requested for empty slice list");
    slice_description const& first = slices.front();
    double lower = first.position - first.thickness / 2;
    slice_description const& last = slices.back();
    double upper = last.position + last.thickness / 2;
    return box1d(make_vector(lower), make_vector(upper - lower));
}

}
