#ifndef CRADLE_GEOMETRY_CLIPPER_HPP
#define CRADLE_GEOMETRY_CLIPPER_HPP

#include <cradle/geometry/polygonal.hpp>
#include <clipper.hpp>

// This file provides an interface between CRADLE's polygonal geometry
// representations and the Clipper library, which provides some useful
// (and very efficient) algorithms.

namespace cradle {

// Since Clipper uses integer coordinates, we need to convert the floating
// point coordinates that CRADLE uses to integers.
// This scale factor determines the relationship between integer values
// and floating point values.
// Since CRADLE spatial coordinates are generally in mm, this gives one micron
// of precision in the integer representation.
double const clipper_integer_precision = 0.001;

typedef ClipperLib::IntPoint clipper_point;

clipper_point to_clipper(vector<2,double> const& p);

vector<2,double> from_clipper(clipper_point const& p);

typedef ClipperLib::Polygon clipper_polygon2;

ClipperLib::Polygon
to_clipper(polygon2 const& poly);

void to_clipper(ClipperLib::Polygon* cp, polygon2 const& poly);

polygon2 from_clipper(ClipperLib::Polygon const& cp);

void from_clipper(polygon2* poly, ClipperLib::Polygon const& cp);

typedef ClipperLib::Polygon clipper_poly;
typedef ClipperLib::Polygons clipper_polyset;

ClipperLib::Polygons
to_clipper(polyset const& cradle_set);

void to_clipper(ClipperLib::Polygons* clipper_set, polyset const& cradle_set);

polyset from_clipper(ClipperLib::Polygons const& clipper_set);

void from_clipper(
    polyset* cradle_set, ClipperLib::Polygons const& clipper_set);

// The following provides the interface to make a Clipper polyset a regular
// CRADLE type. This allows Clipper polysets to be used directly as part of
// the CRADLE API.

raw_type_info get_type_info(clipper_polyset);

bool operator==(clipper_polyset const& a, clipper_polyset const& b);
bool operator!=(clipper_polyset const& a, clipper_polyset const& b);
bool operator<(clipper_polyset const& a, clipper_polyset const& b);

void to_value(value* v, clipper_poly const& x);
void from_value(clipper_poly* x, value const& v);
void to_value(value* v, clipper_polyset const& x);
void from_value(clipper_polyset* x, value const& v);

// hash function
} namespace std {
    template<>
    struct hash<cradle::clipper_polyset>
    {
        size_t operator()(cradle::clipper_polyset const& x) const
        {
            return alia::invoke_hash(cradle::from_clipper(x));
        }
    };
} namespace cradle {

// operations on Clipper polysets

double get_area(clipper_polyset const& set);

clipper_polyset smooth_polyset(clipper_polyset const& set, double smoothSize, double smoothWeight);

}

#endif
