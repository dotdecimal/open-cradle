# find the PostgreSQL api

find_path(PQ_INCLUDE_DIR postgresql/libpq-fe.h
  HINTS "$ENV{PQ_INCLUDE_DIR}"
  PATHS /usr/include /usr/local/include)

find_library(PQ_LIBRARY_RELEASE pq libpq
  HINTS "$ENV{PQ_LIBRARY_DIR}"
  PATHS /usr/lib /usr/local/lib)
find_library(PQ_LIBRARY_DEBUG pqd libpqd
  HINTS "$ENV{PQ_LIBRARY_DIR}"
  PATHS /usr/lib /usr/local/lib)
if(PQ_LIBRARY_DEBUG MATCHES "NOTFOUND")
  set(PQ_LIBRARY ${PQ_LIBRARY_RELEASE})
else()
  set(PQ_LIBRARY
    debug ${PQ_LIBRARY_DEBUG}
    optimized ${PQ_LIBRARY_RELEASE})
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    libpq DEFAULT_MSG PQ_INCLUDE_DIR PQ_LIBRARY)

mark_as_advanced(PQ_INCLUDE_DIR PQ_LIBRARY)

set(PQ_LIBRARIES "${PQ_LIBRARY}")
