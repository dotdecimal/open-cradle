namespace cradle {

template<unsigned N, typename T>
bool has_rotation(matrix<N,N,T> const& m)
{
    for (unsigned i = 0; i < N - 1; ++i)
    {
        for (unsigned j = 0; j < N - 1; ++j)
        {
            if (j != i && !almost_equal<T>(m(i, j), 0))
                return true;
        }
    }
    return false;
}

// Simple matrix pattern matching.
namespace impl { namespace pattern {

    template<typename T>
    struct element
    {
        enum
        {
            // The value should be equal to a given constant.
            CONSTANT,
            // The value should be the same as another value found at a given
            // location.
            SAME_AS,
            // The value should be the negative of another value found at a
            // given location.
            NEGATIVE_OF,
            // Don't care about the value.
            DONT_CARE,
        } type;

        union
        {
            // Used by CONSTANT.
            T constant;
            // Used by SAME_AS and NEGATIVE_OF.
            int offset;
        } data;
    };

    // Convenience functions for creating pattern elements.
    template<typename T>
    element<T> constant(T c)
    {
        element<T> r;
        r.type = element<T>::CONSTANT;
        r.data.constant = c;
        return r;
    }
    template<typename T>
    element<T> same_as(int offset)
    {
        element<T> r;
        r.type = element<T>::SAME_AS;
        r.data.offset = offset;
        return r;
    }
    template<typename T>
    element<T> negative_of(int offset)
    {
        element<T> r;
        r.type = element<T>::NEGATIVE_OF;
        r.data.offset = offset;
        return r;
    }
    template<typename T>
    element<T> dont_care()
    {
        element<T> r;
        r.type = element<T>::DONT_CARE;
        return r;
    }

    // Match an element in a matrix against a pattern element.
    template<typename T, class IteratorT>
    bool match_element(IteratorT begin, IteratorT end, IteratorT value,
        element<T> const& pattern, T epsilon)
    {
        switch (pattern.type)
        {
            case element<T>::CONSTANT:
                return almost_equal(*value, pattern.data.constant,
                    epsilon);
            case element<T>::SAME_AS:
            {
                IteratorT other = value + pattern.data.offset;
                assert(other >= begin && other < end);
                return almost_equal(*value, *other, epsilon);
            }
            case element<T>::NEGATIVE_OF:
            {
                IteratorT other = value + pattern.data.offset;
                assert(other >= begin && other < end);
                return almost_equal(*value, -*other, epsilon);
            }
            default:
                return true;
        }
    }

    // Match a matrix against a pattern matrix.
    template<unsigned M, unsigned N, typename T>
    bool match(matrix<M,N,T> const& values, matrix<M,N,element<T> > const&
        pattern, T tolerance = default_equality_tolerance<T>())
    {
        typename matrix<M,N,T>::const_iterator begin = values.begin();
        typename matrix<M,N,T>::const_iterator end = values.end();
        typename matrix<M,N,element<T> >::const_iterator p = pattern.begin();
        for (typename matrix<M,N,T>::const_iterator i = begin; i != end;
            ++i, ++p)
        {
            if (!match_element(begin, end, i, *p, tolerance))
                return false;
        }
        return true;
    }

}} // namespace impl::pattern

template<typename T>
optional<angle<T,radians> > decode_rotation_about_x(
    matrix<3,3,T> const& m)
{
    using namespace impl::pattern;
    element<T> zero = constant<T>(0), one = constant<T>(1), x = dont_care<T>();
    matrix<3,3,element<T> > p(
        one, zero, zero,
        zero, same_as<T>(+4), negative_of<T>(+2),
        zero, x, x);
    if (!match(m, p))
        return optional<angle<T,radians> >();
    return optional<angle<T,radians> >(atan2(m(2, 1), m(1, 1)));
}

template<typename T>
optional<angle<T,radians> > decode_rotation_about_y(
    matrix<3,3,T> const& m)
{
    using namespace impl::pattern;
    element<T> zero = constant<T>(0), one = constant<T>(1), x = dont_care<T>();
    matrix<3,3,element<T> > p(
        same_as<T>(+8), zero, negative_of<T>(+4),
        zero, one, zero,
        x, zero, x);
    if (!match(m, p))
        return optional<angle<T,radians> >();
    return optional<angle<T,radians> >(atan2(m(0, 2), m(0, 0)));
}

template<typename T>
optional<angle<T,radians> > decode_rotation_about_z(
    matrix<3,3,T> const& m)
{
    using namespace impl::pattern;
    element<T> zero = constant<T>(0), one = constant<T>(1), x = dont_care<T>();
    matrix<3,3,element<T> > p(
        same_as<T>(+4), negative_of<T>(+2), zero,
        x, x, zero,
        zero, zero, one);
    if (!match(m, p))
        return optional<angle<T,radians> >();
    return optional<angle<T,radians> >(atan2(m(1, 0), m(0, 0)));
}

}
