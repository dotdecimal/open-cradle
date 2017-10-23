#ifndef CRADLE_IO_CALC_PROVIDER_HPP
#define CRADLE_IO_CALC_PROVIDER_HPP

#include <cradle/common.hpp>
#include <cradle/api.hpp>

namespace cradle {

// Implement a calculation provider for an API.
void
provide_calculations(
    int argc, char const* const* argv,
    api_implementation const& api);

}

#endif
