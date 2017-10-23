#include <cradle/imaging/isobands.hpp>
#include <cstdio>

using namespace cradle::imaging::impl;

// on edge
isobands_table_vertex vertex(
    char vertex0, char vertex1, char interpolation_level)
{
    isobands_table_vertex v;
    v.on_edge = 1;
    v.vertex0 = vertex0;
    v.vertex1 = vertex1;
    v.interpolation_level = interpolation_level;
    return v;
}

// corner vertex
isobands_table_vertex vertex(char vertex0)
{
    isobands_table_vertex v;
    v.on_edge = 0;
    v.vertex0 = vertex0;
    v.vertex1 = 0;
    v.interpolation_level = 0;
    return v;
}

// 0 vertices
isobands_table_polygon polygon()
{
    isobands_table_polygon p;
    memset(&p, 0, sizeof(p));
    p.n_vertices = 0;
    return p;
}

// 3 vertices
isobands_table_polygon polygon(
    isobands_table_vertex const& v0,
    isobands_table_vertex const& v1,
    isobands_table_vertex const& v2)
{
    isobands_table_polygon p;
    memset(&p, 0, sizeof(p));
    p.n_vertices = 3;
    p.vertices[0] = v0;
    p.vertices[1] = v1;
    p.vertices[2] = v2;
    return p;
}

// 4 vertices
isobands_table_polygon polygon(
    isobands_table_vertex const& v0,
    isobands_table_vertex const& v1,
    isobands_table_vertex const& v2,
    isobands_table_vertex const& v3)
{
    isobands_table_polygon p;
    memset(&p, 0, sizeof(p));
    p.n_vertices = 4;
    p.vertices[0] = v0;
    p.vertices[1] = v1;
    p.vertices[2] = v2;
    p.vertices[3] = v3;
    return p;
}

// 5 vertices
isobands_table_polygon polygon(
    isobands_table_vertex const& v0,
    isobands_table_vertex const& v1,
    isobands_table_vertex const& v2,
    isobands_table_vertex const& v3,
    isobands_table_vertex const& v4)
{
    isobands_table_polygon p;
    memset(&p, 0, sizeof(p));
    p.n_vertices = 5;
    p.vertices[0] = v0;
    p.vertices[1] = v1;
    p.vertices[2] = v2;
    p.vertices[3] = v3;
    p.vertices[4] = v4;
    return p;
}

// 6 vertices
isobands_table_polygon polygon(
    isobands_table_vertex const& v0,
    isobands_table_vertex const& v1,
    isobands_table_vertex const& v2,
    isobands_table_vertex const& v3,
    isobands_table_vertex const& v4,
    isobands_table_vertex const& v5)
{
    isobands_table_polygon p;
    memset(&p, 0, sizeof(p));
    p.n_vertices = 6;
    p.vertices[0] = v0;
    p.vertices[1] = v1;
    p.vertices[2] = v2;
    p.vertices[3] = v3;
    p.vertices[4] = v4;
    p.vertices[5] = v5;
    return p;
}

// 7 vertices
isobands_table_polygon polygon(
    isobands_table_vertex const& v0,
    isobands_table_vertex const& v1,
    isobands_table_vertex const& v2,
    isobands_table_vertex const& v3,
    isobands_table_vertex const& v4,
    isobands_table_vertex const& v5,
    isobands_table_vertex const& v6)
{
    isobands_table_polygon p;
    memset(&p, 0, sizeof(p));
    p.n_vertices = 7;
    p.vertices[0] = v0;
    p.vertices[1] = v1;
    p.vertices[2] = v2;
    p.vertices[3] = v3;
    p.vertices[4] = v4;
    p.vertices[5] = v5;
    p.vertices[6] = v6;
    return p;
}

// 8 vertices
isobands_table_polygon polygon(
    isobands_table_vertex const& v0,
    isobands_table_vertex const& v1,
    isobands_table_vertex const& v2,
    isobands_table_vertex const& v3,
    isobands_table_vertex const& v4,
    isobands_table_vertex const& v5,
    isobands_table_vertex const& v6,
    isobands_table_vertex const& v7)
{
    isobands_table_polygon p;
    memset(&p, 0, sizeof(p));
    p.n_vertices = 8;
    p.vertices[0] = v0;
    p.vertices[1] = v1;
    p.vertices[2] = v2;
    p.vertices[3] = v3;
    p.vertices[4] = v4;
    p.vertices[5] = v5;
    p.vertices[6] = v6;
    p.vertices[7] = v7;
    return p;
}

bool solution_complete(isobands_table_cell* solutions)
{
    // Only two cases are empty (have no vertices): when all vertices are
    // below the lower level and when all vertices are above the higher level.
    // (And there are three cells for each case, because of the center values.)
    int n_empties = 0;
    for (int i = 0; i != 243; ++i)
    {
        if (solutions[i].polys[0].n_vertices == 0)
            ++n_empties;
    }
    return n_empties == 6;
}

// function to flip a solution horizontally
int flip_vertex_index(int index)
{
    return (index & 0x2) | ((index & 1) != 0 ? 0 : 1);
}
isobands_table_vertex flip_vertex(isobands_table_vertex const& v)
{
    isobands_table_vertex r;
    r.on_edge = v.on_edge;
    r.vertex0 = flip_vertex_index(v.vertex0);
    r.vertex1 = flip_vertex_index(v.vertex1);
    r.interpolation_level = v.interpolation_level;
    return r;
}
isobands_table_polygon flip_polygon(isobands_table_polygon const& p)
{
    isobands_table_polygon q;
    memset(&q, 0, sizeof(q));
    q.n_vertices = p.n_vertices;
    for (int i = 0; i != p.n_vertices; ++i)
        q.vertices[(p.n_vertices - 1) - i] = flip_vertex(p.vertices[i]);
    return q;
}

// functions to rotate a solution
int rotate_vertex_index(int index, int amount)
{
    return (index + amount) & 3;
}
isobands_table_vertex rotate_vertex(isobands_table_vertex const& v,
    int amount)
{
    isobands_table_vertex r;
    r.on_edge = v.on_edge;
    r.vertex0 = rotate_vertex_index(v.vertex0, amount);
    r.vertex1 = rotate_vertex_index(v.vertex1, amount);
    r.interpolation_level = v.interpolation_level;
    return r;
}
isobands_table_polygon rotate_polygon(
    isobands_table_polygon const& p, int amount)
{
    isobands_table_polygon q;
    memset(&q, 0, sizeof(q));
    q.n_vertices = p.n_vertices;
    for (int i = 0; i != p.n_vertices; ++i)
        q.vertices[i] = rotate_vertex(p.vertices[i], amount);
    return q;
}

// functions to invert the low/high levels of a solution
int invert_vertex_level(int level)
{
    return 2 - level;
}
int invert_center_level(int level)
{
    return 2 - level;
}
int invert_interpolation_level(int level)
{
    return 1 - level;
}
isobands_table_vertex invert_vertex(isobands_table_vertex const& v)
{
    isobands_table_vertex r;
    r.on_edge = v.on_edge;
    r.vertex0 = v.vertex0;
    r.vertex1 = v.vertex1;
    r.interpolation_level = invert_interpolation_level(v.interpolation_level);
    return r;
}
isobands_table_polygon invert_polygon(isobands_table_polygon const& p)
{
    isobands_table_polygon q;
    memset(&q, 0, sizeof(q));
    q.n_vertices = p.n_vertices;
    for (int i = 0; i != p.n_vertices; ++i)
        q.vertices[i] = invert_vertex(p.vertices[i]);
    return q;
}

void add_single_solution(
    isobands_table_cell* solutions,
    int v0, int v1, int v2, int v3, int center,
    isobands_table_polygon const& poly0, isobands_table_polygon const& poly1)
{
    int solution_index = (((center * 3 + v3) * 3 + v2) * 3 + v1) * 3 + v0;
    assert(solution_index >= 0 && solution_index < 243);
    isobands_table_cell& cell = solutions[solution_index];
    cell.polys[0] = poly0;
    cell.polys[1] = poly1;
}

void add_solution_with_inverse(
    isobands_table_cell* solutions,
    int v0, int v1, int v2, int v3, int center,
    isobands_table_polygon const& poly0, isobands_table_polygon const& poly1)
{
    add_single_solution(solutions, v0, v1, v2, v3, center,
        poly0, poly1);

    add_single_solution(solutions,
        invert_vertex_level(v0),
        invert_vertex_level(v1),
        invert_vertex_level(v2),
        invert_vertex_level(v3),
        invert_center_level(center),
        invert_polygon(poly0), invert_polygon(poly1));
}

void add_solution_with_inverses_and_flips(
    isobands_table_cell* solutions,
    int v0, int v1, int v2, int v3, int center,
    isobands_table_polygon const& poly0, isobands_table_polygon const& poly1)
{
    add_solution_with_inverse(solutions, v0, v1, v2, v3, center,
        poly0, poly1);

    add_solution_with_inverse(solutions,
        v1, v0, v3, v2, center,
        flip_polygon(poly0), flip_polygon(poly1));
}

void add_solution_with_inverses_flips_and_rotations(
    isobands_table_cell* solutions,
    int v0, int v1, int v2, int v3, int center,
    isobands_table_polygon const& poly0, isobands_table_polygon const& poly1)
{
    add_solution_with_inverses_and_flips(
        solutions, v0, v1, v2, v3, center,
        rotate_polygon(poly0, 0), rotate_polygon(poly1, 0));
    add_solution_with_inverses_and_flips(
        solutions, v3, v0, v1, v2, center,
        rotate_polygon(poly0, 1), rotate_polygon(poly1, 1));
    add_solution_with_inverses_and_flips(
        solutions, v2, v3, v0, v1, center,
        rotate_polygon(poly0, 2), rotate_polygon(poly1, 2));
    add_solution_with_inverses_and_flips(
        solutions, v1, v2, v3, v0, center,
        rotate_polygon(poly0, 3), rotate_polygon(poly1, 3));
}

void add_solutions(
    isobands_table_cell* solutions,
    int v0, int v1, int v2, int v3, int center_min, int center_max,
    isobands_table_polygon const& poly0 = polygon(),
    isobands_table_polygon const& poly1 = polygon())
{
    for (int i = center_min; i <= center_max; ++i)
    {
        add_solution_with_inverses_flips_and_rotations(
            solutions, v0, v1, v2, v3, i, poly0, poly1);
    }
}

void initialize_isobands_solution_table(
    isobands_table_cell* solutions)
{
    for (int i = 0; i != 243; ++i)
    {
        solutions[i].polys[0].n_vertices =
            solutions[i].polys[1].n_vertices = 0;
    }

    add_solutions(solutions, 0, 0, 0, 0, 0, 2);
    add_solutions(solutions, 1, 0, 0, 0, 0, 2,
        polygon(
            vertex(0),
            vertex(0, 1, 0),
            vertex(3, 0, 0)));
    add_solutions(solutions, 1, 1, 0, 0, 0, 2,
        polygon(
            vertex(0),
            vertex(1),
            vertex(1, 2, 0),
            vertex(3, 0, 0)));
    add_solutions(solutions, 1, 0, 1, 0, 0, 0,
        polygon(
            vertex(0),
            vertex(0, 1, 0),
            vertex(3, 0, 0)),
        polygon(
            vertex(1, 2, 0),
            vertex(2),
            vertex(2, 3, 0)));
    add_solutions(solutions, 1, 0, 1, 0, 1, 2,
        polygon(
            vertex(0),
            vertex(0, 1, 0),
            vertex(1, 2, 0),
            vertex(2),
            vertex(2, 3, 0),
            vertex(3, 0, 0)));
    add_solutions(solutions, 0, 1, 1, 1, 0, 2,
        polygon(
            vertex(0, 1, 0),
            vertex(1),
            vertex(2),
            vertex(3),
            vertex(3, 0, 0)));
    add_solutions(solutions, 1, 1, 1, 1, 0, 2,
        polygon(
            vertex(0),
            vertex(1),
            vertex(2),
            vertex(3)));
    add_solutions(solutions, 0, 1, 2, 2, 0, 2,
        polygon(
            vertex(0, 1, 0),
            vertex(1),
            vertex(1, 2, 1),
            vertex(3, 0, 1),
            vertex(3, 0, 0)));
    add_solutions(solutions, 0, 1, 2, 2, 0, 2,
        polygon(
            vertex(0, 1, 0),
            vertex(1),
            vertex(1, 2, 1),
            vertex(3, 0, 1),
            vertex(3, 0, 0)));
    add_solutions(solutions, 0, 1, 0, 2, 0, 0,
        polygon(
            vertex(0, 1, 0),
            vertex(1),
            vertex(1, 2, 0)),
        polygon(
            vertex(2, 3, 0),
            vertex(2, 3, 1),
            vertex(3, 0, 1),
            vertex(3, 0, 0)));
    add_solutions(solutions, 0, 1, 0, 2, 1, 2,
        polygon(
            vertex(0, 1, 0),
            vertex(1),
            vertex(1, 2, 0),
            vertex(2, 3, 0),
            vertex(2, 3, 1),
            vertex(3, 0, 1),
            vertex(3, 0, 0)));
    add_solutions(solutions, 0, 2, 0, 2, 0, 0,
        polygon(
            vertex(0, 1, 0),
            vertex(0, 1, 1),
            vertex(1, 2, 1),
            vertex(1, 2, 0)),
        polygon(
            vertex(2, 3, 0),
            vertex(2, 3, 1),
            vertex(3, 0, 1),
            vertex(3, 0, 0)));
    add_solutions(solutions, 0, 2, 0, 2, 1, 1,
        polygon(
            vertex(0, 1, 0),
            vertex(0, 1, 1),
            vertex(1, 2, 1),
            vertex(1, 2, 0),
            vertex(2, 3, 0),
            vertex(2, 3, 1),
            vertex(3, 0, 1),
            vertex(3, 0, 0)));
    add_solutions(solutions, 0, 2, 0, 2, 2, 2,
        polygon(
            vertex(0, 1, 0),
            vertex(0, 1, 1),
            vertex(3, 0, 1),
            vertex(3, 0, 0)),
        polygon(
            vertex(2, 3, 0),
            vertex(2, 3, 1),
            vertex(1, 2, 1),
            vertex(1, 2, 0)));
    add_solutions(solutions, 2, 0, 0, 0, 0, 2,
        polygon(
            vertex(0, 1, 1),
            vertex(0, 1, 0),
            vertex(3, 0, 0),
            vertex(3, 0, 1)));
    add_solutions(solutions, 2, 2, 0, 0, 0, 2,
        polygon(
            vertex(1, 2, 1),
            vertex(1, 2, 0),
            vertex(3, 0, 0),
            vertex(3, 0, 1)));
    add_solutions(solutions, 0, 1, 1, 2, 0, 2,
        polygon(
            vertex(0, 1, 0),
            vertex(1),
            vertex(2),
            vertex(2, 3, 1),
            vertex(3, 0, 1),
            vertex(3, 0, 0)));
    add_solutions(solutions, 0, 1, 2, 1, 0, 2,
        polygon(
            vertex(0, 1, 0),
            vertex(1),
            vertex(1, 2, 1),
            vertex(2, 3, 1),
            vertex(3),
            vertex(3, 0, 0)));

    assert(solution_complete(solutions));
}

int main()
{
    isobands_table_cell table[243];
    memset(table, 0, sizeof(table));
    initialize_isobands_solution_table(table);
    printf("isobands_table_cell isobands_table[243] = {\n");
    for (int i = 0; i != 243; ++i)
    {
        isobands_table_cell const& cell = table[i];
        printf("    {\n");
        printf("        {\n");
        for (int j = 0; j != 2; ++j)
        {
            isobands_table_polygon const& poly = cell.polys[j];
            printf("            {\n");
            printf("                %i,\n", poly.n_vertices);
            printf("                {\n");
            for (int k = 0; k != 8; ++k)
            {
                isobands_table_vertex const& v = poly.vertices[k];
                printf("                    { %u, %u, %u, %u }",
                    v.on_edge, v.vertex0, v.vertex1, v.interpolation_level);
                if (k != 7)
                    printf(",");
                printf("\n");
            }
            printf("                }\n");
            printf("            }");
            if (j != 1)
                printf(",");
            printf("\n");
        }
        printf("        }");
        printf("    }");
        if (i != 242)
            printf(",");
        printf("\n");
    }
    printf("};\n");
}
