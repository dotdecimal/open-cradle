# find COIN-OR's Clp module

find_path(CLP_INCLUDE_DIR coin/ClpSimplex.hpp
  PATHS /usr/include /usr/local/include)

find_library(CLP_DEBUG_LIBRARY Clp_debug libClp_debug Clp libClp
    PATHS /usr/lib /usr/local/lib)
find_library(CLP_RELEASE_LIBRARY Clp libClp
    PATHS /usr/lib /usr/local/lib)

find_library(COIN_UTILS_DEBUG_LIBRARY CoinUtils_debug libCoinUtils_debug
    CoinUtils libCoinUtils PATHS /usr/lib /usr/local/lib)
find_library(COIN_UTILS_RELEASE_LIBRARY CoinUtils libCoinUtils
    PATHS /usr/lib /usr/local/lib)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    Clp DEFAULT_MSG CLP_DEBUG_LIBRARY CLP_RELEASE_LIBRARY
    COIN_UTILS_DEBUG_LIBRARY COIN_UTILS_RELEASE_LIBRARY CLP_INCLUDE_DIR)

mark_as_advanced(CLP_INCLUDE_DIR CLP_DEBUG_LIBRARY CLP_RELEASE_LIBRARY
    COIN_UTILS_DEBUG_LIBRARY COIN_UTILS_RELEASE_LIBRARY)

set(CLP_LIBRARIES
    optimized "${CLP_RELEASE_LIBRARY}"
    optimized "${COIN_UTILS_RELEASE_LIBRARY}"
    debug "${CLP_DEBUG_LIBRARY}"
    debug "${COIN_UTILS_DEBUG_LIBRARY}")
