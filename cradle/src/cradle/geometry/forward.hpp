#ifndef CRADLE_GEOMETRY_FORWARD_HPP
#define CRADLE_GEOMETRY_FORWARD_HPP

#include <vector>

// Many geometry objects are templated over dimensionality and scalar type.
// This macro will define the most common typedefs for such a type, using the
// standard CRADLE naming conventions...
#define CRADLE_GEOMETRY_DEFINE_TYPEDEFS(type) \
    typedef type<0, int> type##0i; \
    typedef type<1, int> type##1i; \
    typedef type<2, int> type##2i; \
    typedef type<3, int> type##3i; \
    typedef type<4, int> type##4i; \
    typedef type<0, unsigned> type##0u; \
    typedef type<1, unsigned> type##1u; \
    typedef type<2, unsigned> type##2u; \
    typedef type<3, unsigned> type##3u; \
    typedef type<4, unsigned> type##4u; \
    typedef type<0, float> type##0f; \
    typedef type<1, float> type##1f; \
    typedef type<2, float> type##2f; \
    typedef type<3, float> type##3f; \
    typedef type<4, float> type##4f; \
    typedef type<0, double> type##0d; \
    typedef type<1, double> type##1d; \
    typedef type<2, double> type##2d; \
    typedef type<3, double> type##3d; \
    typedef type<4, double> type##4d;
// Same, but only floating point types.
#define CRADLE_GEOMETRY_DEFINE_FLOATING_TYPEDEFS(type) \
    typedef type<0, float> type##0f; \
    typedef type<1, float> type##1f; \
    typedef type<2, float> type##2f; \
    typedef type<3, float> type##3f; \
    typedef type<4, float> type##4f; \
    typedef type<0, double> type##0d; \
    typedef type<1, double> type##1d; \
    typedef type<2, double> type##2d; \
    typedef type<3, double> type##3d; \
    typedef type<4, double> type##4d;

namespace alia {

template<unsigned N, class T>
struct vector;

template<unsigned N, class T>
struct box;

}

namespace cradle {

// from alia
using alia::vector;
CRADLE_GEOMETRY_DEFINE_TYPEDEFS(vector)
using alia::box;
CRADLE_GEOMETRY_DEFINE_TYPEDEFS(box)

// meshing.hpp
struct optimized_triangle_mesh;
struct triangle_mesh;
struct triangle_mesh_with_normals;

// polygonal.hpp
struct polygon2;
struct polygon_with_holes;
struct polyset;
struct structure_geometry;

// regular_grid.hpp
template<unsigned N, class T>
struct regular_grid;

// scenes.hpp
template<unsigned N>
struct sliced_scene_geometry;

// slicing.hpp
struct slice_description;
typedef std::vector<slice_description> slice_description_list;

}

#endif
