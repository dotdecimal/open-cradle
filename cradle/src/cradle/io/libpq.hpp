#ifndef CRADLE_IO_LIBPQ_HPP
#define CRADLE_IO_LIBPQ_HPP

#include <cradle/external/libpq.hpp>
#include <cradle/io/postgresql.hpp>

// Utility classes/functions for working with libpq.

namespace cradle {

class scoped_result
{
 public:
    scoped_result(PGresult* r) : r_(r) {}
    ~scoped_result() { if (r_) PQclear(r_); }
    PGresult* get() const { return r_; }
 private:
    PGresult* r_;
};

static inline PGconn* get_pgconn(connection const& conn)
{ return reinterpret_cast<PGconn*>(conn.get()); }

// These are from pg_types.h, which is very difficult to include itself.
#define NULLOID        0
#define BOOLOID        16
#define BYTEAOID       17
#define CHAROID        18
#define NAMEOID        19
#define INT8OID        20
#define INT2OID        21
#define INT2VECTOROID  22
#define INT4OID        23
#define REGPROCOID     24
#define TEXTOID        25
#define OIDOID         26
#define TIDOID         27
#define XIDOID         28
#define CIDOID         29
#define OIDVECTOROID   30
#define POINTOID       600
#define LSEGOID        601
#define PATHOID        602
#define BOXOID         603
#define POLYGONOID     604
#define LINEOID        628
#define FLOAT4OID      700
#define FLOAT8OID      701
#define ABSTIMEOID     702
#define RELTIMEOID     703
#define TINTERVALOID   704
#define UNKNOWNOID     705
#define CIRCLEOID      718
#define CASHOID        790
#define INETOID        869
#define CIDROID        650
#define BPCHAROID      1042
#define VARCHAROID     1043
#define DATEOID        1082
#define TIMEOID        1083
#define TIMESTAMPOID   1114
#define TIMESTAMPTZOID 1184
#define INTERVALOID    1186
#define TIMETZOID      1266
#define ZPBITOID       1560
#define VARBITOID      1562
#define NUMERICOID     1700

}

#endif
