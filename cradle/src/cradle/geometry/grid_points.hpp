#ifndef CRADLE_GEOMETRY_GRID_POINTS_HPP
#define CRADLE_GEOMETRY_GRID_POINTS_HPP

#include <cradle/geometry/regular_grid.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <vector>

namespace cradle {

// A regular_grid_iterator is used for traversing the points on a regular_grid.
template<unsigned N, typename T>
class regular_grid_iterator
  : public boost::iterator_facade<
        regular_grid_iterator<N,T>
      , vector<N,T> const
      , boost::random_access_traversal_tag
    >
{
 public:
    regular_grid_iterator() {}
    regular_grid_iterator(regular_grid<N,T> const* grid,
        vector<N,unsigned> const& i, vector<N,T> const& p)
      : grid(grid), i(i), p(p)
    {}

 private:
    friend class boost::iterator_core_access;

    vector<N,T> const& dereference() const { return p; }

    bool equal(regular_grid_iterator const& other) const
    { return this->i == other.i; }

    void increment();
    void decrement();

    void advance(typename regular_grid_iterator::difference_type n);

    typename regular_grid_iterator::difference_type
    distance_to(regular_grid_iterator const& other) const;

    regular_grid<N,T> const* grid;
    vector<N,unsigned> i;
    vector<N,T> p;
};

template<unsigned N, typename T>
class regular_grid_point_list
{
 public:
    regular_grid_point_list() {}
    regular_grid_point_list(regular_grid<N,T> const& grid) : grid_(grid) {}

    typedef regular_grid_iterator<N,T> iterator;
    typedef iterator const_iterator;

    const_iterator begin() const
    {
        return const_iterator(&grid_, uniform_vector<N,unsigned>(0),
            grid_.p0);
    }
    const_iterator end() const
    {
        vector<N,unsigned> i = uniform_vector<N,unsigned>(0);
        i[N - 1] = grid_.n_points[N - 1];
        return const_iterator(&grid_, i,
            grid_.p0 + cradle::vector<N,T>(i) * grid_.spacing);
    }

    typedef size_t size_type;
    size_type size() const { return product(grid_.n_points); }

 private:
    regular_grid<N,T> grid_;
};

template<unsigned N, typename T>
regular_grid_point_list<N,T>
make_grid_point_list(regular_grid<N,T> const& grid)
{ return regular_grid_point_list<N,T>(grid); }

// Get the list of points on a grid.
api(fun with(N:1,2,3,4; T:double))
template<unsigned N, typename T>
// List of points on the N dimensional grid, with N being 1 thru 4
std::vector<cradle::vector<N,T> >
get_points_on_grid(
// The grid over which to compute the point positions
regular_grid<N,T> const& grid)
{
    auto lazy = make_grid_point_list(grid);
    std::vector<cradle::vector<N,T> > eager;
    eager.reserve(lazy.size());
    for (auto i = lazy.begin(); i != lazy.end(); ++i)
        eager.push_back(*i);
    return eager;
}

namespace impl {

    template<unsigned N, typename T, unsigned I_>
    struct regular_grid_iterator_incrementer
    {
        static void apply(
            regular_grid<N,T> const& grid,
            vector<N,unsigned>& i,
            vector<N,T>& p)
        {
            static unsigned const I = N - I_;
            ++i[I];
            if (i[I] == grid.n_points[I])
            {
                i[I] = 0;
                p[I] = grid.p0[I];
                regular_grid_iterator_incrementer<N,T,N - (I + 1)>::apply(
                    grid, i, p);
            }
            else
                p[I] += grid.spacing[I];
        }
    };
    template<unsigned N, typename T>
    struct regular_grid_iterator_incrementer<N,T,1>
    {
        static void apply(
            regular_grid<N,T> const& grid,
            vector<N,unsigned>& i,
            vector<N,T>& p)
        {
            static unsigned const I = N - 1;
            ++i[I];
            p[I] += grid.spacing[I];
        }
    };
}

template<unsigned N, typename T>
void regular_grid_iterator<N,T>::increment()
{
    impl::regular_grid_iterator_incrementer<N,T,N>::apply(*grid, i, p);
}

namespace impl {

    template<unsigned N, typename T, unsigned I_>
    struct regular_grid_iterator_decrementer
    {
        static void apply(
            regular_grid<N,T> const& grid,
            vector<N,unsigned>& i,
            vector<N,T>& p)
        {
            static unsigned const I = N - I_;
            if (i[I] == 0)
            {
                i[I] = grid.n_points[I] - 1;
                p[I] = grid.p0[I] + i[I] * grid.spacing[I];
                regular_grid_iterator_decrementer<N,T,N - (I + 1)>::apply(
                    grid, i, p);
            }
            else
            {
                --i[I];
                p[I] -= grid.spacing[I];
            }
        }
    };
    template<unsigned N, typename T>
    struct regular_grid_iterator_decrementer<N,T,1>
    {
        static void apply(
            regular_grid<N,T> const& grid,
            vector<N,unsigned>& i,
            vector<N,T>& p)
        {
            static unsigned const I = N - 1;
            --i[I];
            p[I] -= grid.spacing[I];
        }
    };
}

template<unsigned N, typename T>
void regular_grid_iterator<N,T>::decrement()
{
    impl::regular_grid_iterator_decrementer<N,T,N>::apply(*grid, i, p);
}

namespace impl {

    template<unsigned N, typename T, unsigned I_>
    struct regular_grid_iterator_advancer
    {
        template<typename Difference>
        static void apply(
            regular_grid<N,T> const& grid,
            vector<N,unsigned>& i,
            vector<N,T>& p,
            Difference n)
        {
            static unsigned const I = N - I_;
            Difference t = static_cast<Difference>(i[I]) + n;
            Difference r = t % static_cast<Difference>(grid.n_points[I]);
            Difference q = t / static_cast<Difference>(grid.n_points[I]);
            if (r < 0)
            {
                r = grid.n_points[I] - r;
                --q;
            }
            i[I] = unsigned(r);
            p[I] = grid.p0[I] + i[I] * grid.spacing[I];
            if (q != 0)
            {
                regular_grid_iterator_advancer<N,T,N - (I + 1)>::apply(
                    grid, i, p, q);
            }
        }
    };
    template<unsigned N, typename T>
    struct regular_grid_iterator_advancer<N,T,1>
    {
        template<typename Difference>
        static void apply(
            regular_grid<N,T> const& grid,
            vector<N,unsigned>& i,
            vector<N,T>& p,
            Difference n)
        {
            static unsigned const I = N - 1;
            i[I] += unsigned(n);
            p[I] = grid.p0[I] + i[I] * grid.spacing[I];
        }
    };
}

template<unsigned N, typename T>
void regular_grid_iterator<N,T>::advance(
    typename regular_grid_iterator<N,T>::difference_type n)
{
    impl::regular_grid_iterator_advancer<N,T,N>::apply(*grid, i, p, n);
}

namespace impl {

    template<typename Difference, unsigned N, typename T, unsigned I_>
    struct regular_grid_iterator_distance
    {
        static Difference apply(
            regular_grid<N,T> const& grid,
            vector<N,unsigned> const& a,
            vector<N,unsigned> const& b)
        {
            static unsigned const I = N - I_;
            return (static_cast<Difference>(b[I]) -
                static_cast<Difference>(a[I])) +
                regular_grid_iterator_distance<Difference,N,T,N - (I + 1)>
                    ::apply(grid, a, b) * grid.n_points[I];
        }
    };
    template<typename Difference, unsigned N, typename T>
    struct regular_grid_iterator_distance<Difference,N,T,1>
    {
        static Difference apply(
            regular_grid<N,T> const& grid,
            vector<N,unsigned> const& a,
            vector<N,unsigned> const& b)
        {
            static unsigned const I = N - 1;
            return (static_cast<Difference>(b[I]) -
                static_cast<Difference>(a[I]));
        }
    };
}

template<unsigned N, typename T>
auto
regular_grid_iterator<N,T>::distance_to(
    regular_grid_iterator const& other) const
 -> typename regular_grid_iterator::difference_type
{
    assert(*grid == *other.grid);
    return impl::regular_grid_iterator_distance<
        typename regular_grid_iterator<N,T>::difference_type,N,T,N>::apply(
        *grid, i, other.i);
}

}

#endif
