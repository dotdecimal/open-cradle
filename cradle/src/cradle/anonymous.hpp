#ifndef CRADLE_ANONYMOUS_HPP
#define CRADLE_ANONYMOUS_HPP

#include <boost/preprocessor/repetition.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/call_traits.hpp>

namespace cradle {

// This function is used to create anonymous lists, vectors and other container
// values. It saves you from having to create the container, name it, and
// repeatedly push values to it.

#define CRADLE_ANONYMOUS_MAX_ARGS 12

#define p(z, n, unused) \
    container.push_back(obj##n);

#define fn(z, n, unused) \
    template<class ContainerT> \
    ContainerT anonymous(BOOST_PP_ENUM_PARAMS(n, \
        typename boost::call_traits< \
            typename ContainerT::value_type>::param_type obj)) \
    { \
        ContainerT container; \
        BOOST_PP_REPEAT(n, p, ~) \
        return container; \
    }

BOOST_PP_REPEAT(BOOST_PP_ADD(CRADLE_ANONYMOUS_MAX_ARGS, 1), fn, ~)

#undef p
#undef fn

}

#endif
