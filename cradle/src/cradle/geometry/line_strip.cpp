#include <cradle/geometry/line_strip.hpp>
#include <algorithm>
#include <list>

namespace cradle {

// When adding a point to a line strip in a set of strips, it's possible that
// that point already ends another line strip, in which case it the two strips
// should be connected together.  The following two functions test for and
// handle that case.

bool attempt_to_append_strip(std::list<std::list<vector2d> >& strips,
    std::list<vector2d>& strip, vector2d const& p, double tolerance)
{
    for (std::list<std::list<vector2d> >::iterator
        i = strips.begin(); i != strips.end(); ++i)
    {
        if (&*i == &strip)
            continue;
        if (almost_equal(i->back(), p, tolerance))
        {
            i->splice(i->end(), strip);
            return true;
        }
        if (almost_equal(i->front(), p, tolerance))
        {
            if (i->size() > strip.size())
            {
                strip.reverse();
                i->splice(i->begin(), strip);
            }
            else
            {
                i->reverse();
                i->splice(i->end(), strip);
            }
            return true;
        }
    }
    return false;
}

bool attempt_to_prepend_strip(std::list<std::list<vector2d> >& strips,
    std::list<vector2d>& strip, vector2d const& p, double tolerance)
{
    for (std::list<std::list<vector2d> >::iterator
        i = strips.begin(); i != strips.end(); ++i)
    {
        if (&*i == &strip)
            continue;
        if (almost_equal(i->front(), p, tolerance))
        {
            i->splice(i->begin(), strip);
            return true;
        }
        if (almost_equal(i->back(), p, tolerance))
        {
            if (i->size() > strip.size())
            {
                strip.reverse();
                i->splice(i->end(), strip);
            }
            else
            {
                i->reverse();
                i->splice(i->begin(), strip);
            }
            return true;
        }
    }
    return false;
}

void add_segment_to_strip_set(std::list<std::list<vector2d> >& strips,
    vector2d const& p0, vector2d const& p1, double tolerance)
{
    // Look for an existing strip that begins or ends with one of the end
    // points of the new segment. If one is found, the segment should be
    // connected to that strip.
    for (std::list<std::list<vector2d> >::iterator
        i = strips.begin(); i != strips.end(); ++i)
    {
        if (almost_equal(i->front(), p0, tolerance))
        {
            if (attempt_to_append_strip(strips, *i, p1, tolerance))
                strips.erase(i);
            else
                i->push_front(p1);
            return;
        }
        if (almost_equal(i->front(), p1, tolerance))
        {
            if (attempt_to_append_strip(strips, *i, p0, tolerance))
                strips.erase(i);
            else
                i->push_front(p0);
            return;
        }
        if (almost_equal(i->back(), p0, tolerance))
        {
            if (attempt_to_prepend_strip(strips, *i, p1, tolerance))
                strips.erase(i);
            else
                i->push_back(p1);
            return;
        }
        if (almost_equal(i->back(), p1, tolerance))
        {
            if (attempt_to_prepend_strip(strips, *i, p0, tolerance))
                strips.erase(i);
            else
                i->push_back(p0);
            return;
        }
    }

    // Couldn't add it to an existing strip, so create a new one.
    std::list<vector2d> new_strip;
    new_strip.push_back(p0);
    new_strip.push_back(p1);
    strips.push_back(new_strip);
}

std::vector<line_strip>
connect_line_segments(
    std::vector<line_segment<2,double> > const& segments,
    double tolerance)
{
    std::list<std::list<vector2d> > strips;
    for (std::vector<line_segment<2,double> >::const_iterator
        i = segments.begin(); i != segments.end(); ++i)
    {
        add_segment_to_strip_set(strips, (*i)[0], (*i)[1], tolerance);
    }

    std::vector<line_strip> result;
    result.resize(strips.size());
    std::vector<line_strip>::iterator result_i = result.begin();
    for (std::list<std::list<vector2d> >::const_iterator
        i = strips.begin(); i != strips.end(); ++i, ++result_i)
    {
        result_i->vertices.resize(i->size());
        std::copy(i->begin(), i->end(), result_i->vertices.begin());
    }
    return result;
}

bool is_polygon(std::vector<line_strip> const& strips, double tolerance)
{
    if (strips.size() != 1)
        return false;
    line_strip const& strip = strips.front();
    return strip.vertices.size() > 3 &&
        almost_equal(strip.vertices.front(), strip.vertices.back(), tolerance);
}

optional<polygon2> as_polygon(
    std::vector<line_strip> const& strips, double tolerance)
{
    optional<polygon2> result;
    as_polygon(&result, strips, tolerance);
    return result;
}

void as_polygon(optional<polygon2>* result,
    std::vector<line_strip> const& strips, double tolerance)
{
    if (strips.size() != 1)
    {
        *result = optional<polygon2>();
        return;
    }
    line_strip const& strip = strips.front();
    if (strip.vertices.size() < 4 ||
        !almost_equal(strip.vertices.front(), strip.vertices.back(),
            tolerance))
    {
        *result = optional<polygon2>();
        return;
    }

    *result = polygon2();
    copy(strip.vertices.begin(), strip.vertices.end() - 1,
        allocate(&get(*result).vertices, strip.vertices.size() - 1));
}

bool is_polyset(std::vector<line_strip> const& strips, double tolerance)
{
    for (std::vector<line_strip>::const_iterator
        i = strips.begin(); i != strips.end(); ++i)
    {
        if (i->vertices.size() < 4 ||
            !almost_equal(i->vertices.front(), i->vertices.back(), tolerance))
        {
            return false;
        }
    }
    return true;
}

optional<polyset> as_polyset(
    std::vector<line_strip> const& strips, double tolerance)
{
    optional<polyset> result;
    as_polyset(&result, strips, tolerance);
    return result;
}

void as_polyset(optional<polyset>* result,
    std::vector<line_strip> const& strips, double tolerance)
{
    polyset tmp;
    for (std::vector<line_strip>::const_iterator
        i = strips.begin(); i != strips.end(); ++i)
    {
        if (i->vertices.size() < 4 ||
            !almost_equal(i->vertices.front(), i->vertices.back(), tolerance))
        {
            *result = optional<polyset>();
            return;
        }
        polygon2 poly;
        copy(i->vertices.begin(), i->vertices.end() - 1,
            allocate(&poly.vertices, i->vertices.size() - 1));
        cradle::polyset region;
        create_polyset(&region, poly);
        do_set_operation(&tmp, set_operation::XOR, tmp, region);
    }
    *result = polyset();
    swap(result->get(), tmp);
}

}
