#ifndef CRADLE_TEST_HPP
#define CRADLE_TEST_HPP

#include <cradle/io/file.hpp>
#include <algorithm>
#include <cradle/math/common.hpp>

#ifdef __GNUC__
#define BOOST_TEST_DYN_LINK
#endif
#include <boost/test/unit_test.hpp>

namespace cradle {

// Get the directory that contains the test data.
file_path test_data_directory();

}

// Check that the given expressions are almost equal.
#define CRADLE_CHECK_ALMOST_EQUAL(expr1, expr2) \
  { \
      using cradle::almost_equal; \
      if(!almost_equal(expr1, expr2)) \
      { \
          BOOST_ERROR("CRADLE_CHECK_ALMOST_EQUAL() failed" \
              "\n    " #expr1 " != " #expr2 \
              "\n    " << expr1 << " != " << expr2); \
      } \
  }

// Check that the given expressions are within the given tolerance.
#define CRADLE_CHECK_WITHIN_TOLERANCE(expr1, expr2, tolerance) \
  { \
      using cradle::almost_equal; \
      if(!almost_equal(expr1, expr2, tolerance)) \
      { \
          BOOST_ERROR("CRADLE_CHECK_WITHIN_TOLERANCE() failed" \
              "\n    " #expr1 " != " #expr2 \
              "\n    " << expr1 << " != " << expr2 << \
              "\n    tolerance: " << tolerance); \
      } \
  }

// Check that the given ranges are equal.
#define CRADLE_CHECK_RANGES_EQUAL_(begin1, end1, begin2, end2) \
  { if (std::distance(begin1, end1) != std::distance(begin2, end2)) \
    { \
        BOOST_ERROR("CRADLE_CHECK_RANGES_EQUAL() failed" \
            "\n    (" #begin1 ", " #end1 ") != (" #begin2 ", " #end2 ")" \
            "\n    sizes: " << std::distance(begin1, end1) << " != " \
            << std::distance(begin2, end2)); \
    } \
    else if (std::mismatch(begin1, end1, begin2).first != end1) \
    { \
        BOOST_ERROR("CRADLE_CHECK_RANGES_EQUAL() failed" \
            "\n    (" #begin1 ", " #end1 "] != (" #begin2 ", " #end2 "]" \
            "\n    at index " << \
            std::distance(begin1, std::mismatch(begin1, end1, begin2).first) \
            << ": " \
            << *std::mismatch(begin1, end1, begin2).first << " != " \
            << *std::mismatch(begin1, end1, begin2).second); \
    } \
  }
#define CRADLE_CHECK_RANGES_EQUAL(range1, range2) \
    CRADLE_CHECK_RANGES_EQUAL_(range1.begin(), range1.end(), \
        range2.begin(), range2.end())

namespace cradle { namespace impl {

    template<class IteratorT1, class IteratorT2>
    bool check_ranges_almost_equal(
        IteratorT1 const& begin1, IteratorT1 const& end1,
        IteratorT2 const& begin2, IteratorT2 const& end2,
        string& error)
    {
        IteratorT1 i1 = begin1;
        IteratorT2 i2 = begin2;
        for (; i1 != end1; ++i1, ++i2)
        {
            using cradle::almost_equal;
            if (!almost_equal(*i1, *i2))
            {
                error = "at index " +
                    to_string(std::distance(begin1, i1)) + ": " +
                    to_string(*i1) + " != " + to_string(*i2);
                return false;
            }
        }
        return true;
    }
}}

// Check that the given ranges are almost equal.
#define CRADLE_CHECK_RANGES_ALMOST_EQUAL_(begin1, end1, begin2, end2) \
  { if (std::distance(begin1, end1) != std::distance(begin2, end2)) \
    { \
        BOOST_ERROR("CRADLE_CHECK_RANGES_ALMOST_EQUAL() failed" \
            "\n    (" #begin1 ", " #end1 ") != (" #begin2 ", " #end2 ")" \
            "\n    sizes: " << std::distance(begin1, end1) << " != " \
            << std::distance(begin2, end2)); \
    } \
    else \
    { \
        std::string msg;                                                \
        if (!cradle::impl::check_ranges_almost_equal(begin1, end1, begin2, \
            end2, msg)) \
        { \
            BOOST_ERROR("CRADLE_CHECK_RANGES_ALMOST_EQUAL() failed" \
                "\n    (" #begin1 ", " #end1 ") != (" #begin2 ", " #end2 ")" \
                << msg); \
        } \
    } \
  }
#define CRADLE_CHECK_RANGES_ALMOST_EQUAL(range1, range2) \
    CRADLE_CHECK_RANGES_ALMOST_EQUAL_(range1.begin(), range1.end(), \
        range2.begin(), range2.end())

#endif
