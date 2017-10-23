#ifndef CRADLE_GEOMETRY_LINE_STRIP_HPP
#define CRADLE_GEOMETRY_LINE_STRIP_HPP

#include <cradle/geometry/polygonal.hpp>

namespace cradle {

// A line strip is a list of vertices that are connected in order.
// Note that the last vertex is not implicitly connected back to the first,
// and thus the strip is not necessarily closed.
api(struct)
struct line_strip
{
    std::vector<vector<2,double> > vertices;
};

// Given a list of line segments, some of which may share vertices, this will
// merge them along their shared vertices into a list of line strips.
// If the tolerance parameter is non-zero, the algorithm will merge vertices
// even if they're not exactly equal, as long as they're within the specified
// tolerance.
api(fun)
// List of line strips resulting from the merged line segments
std::vector<line_strip>
connect_line_segments(
    // The list of line segments that are to be merged
    std::vector<line_segment<2,double> > const& segments,
    // The tolerence parameter for merging
    double tolerance);

// The following functions will analyze the connectivity of the list of line
// strips returned by connect_line_segments. Note that given the inexact nature
// of the connect_line_segments function, especially with a non-zero tolerance,
// it's not recommended that you use this for any real data analysis. It's
// currently only used for unit testing other code.

// polygon

bool is_polygon(std::vector<line_strip> const& strips, double tolerance);

optional<polygon2> as_polygon(
    std::vector<line_strip> const& strips, double tolerance);

void as_polygon(optional<polygon2>* result,
    std::vector<line_strip> const& strips, double tolerance);

// polyset

bool is_polyset(std::vector<line_strip> const& strips, double tolerance);

optional<polyset> as_polyset(
    std::vector<line_strip> const& strips, double tolerance);

void as_polyset(optional<polyset>* result,
    std::vector<line_strip> const& strips, double tolerance);

}

#endif
