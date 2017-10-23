/*
 * Author(s):  Daniel Bottomley <dbottomley@dotdecimal.com>, Kevin Erhart
 * Date:       09/15/2014
 *
 */

#ifndef CRADLE_UNIT_TESTS_HPP
#define CRADLE_UNIT_TESTS_HPP

#include <alia/common.hpp>
#include <cradle/date_time.hpp>
#include <fstream>
#include <cmath>

namespace cradle{

// Test variables
struct unit_test
{
        int passedCount;
        int failedCount;
        double tol;
        string appName;
        cradle::time startTime;
        std::ofstream stream;

        unit_test(double tolerance, string app_name, cradle::time start_time, string log_file)
                :passedCount(0), failedCount(0), tol(tolerance), appName(app_name), startTime(start_time)
        {
                stream.open(log_file);
        }
};

template <class T, unsigned N>
bool are_equal(const vector<N,T> &a, const vector<N,T> &b, const T tol = T())
{
    for (unsigned i = 0; i < N; ++i)
    {
        if (std::abs(a[i] - b[i]) > tol) { return false; }
    }
        return true;
}

template <class T>
bool are_equal(const T a, const T b, const T tol = T())
{
        return (std::abs(a - b) <= tol);
}

template <class T>
bool are_equal(const T a, const T b, const T tol, unit_test &ut, const wchar_t* message)
{
        bool result = (std::abs(a - b) <= tol);
        if (!result)
        {
                ut.stream << "--FAIL--" << *message << "\n";
                std::cout << "--FAIL--" << *message << std::endl;
        }
        return result;
}

void write_test_header(unit_test &ut);

void write_section_header(unit_test &ut, string sectionName);

void write_result(unit_test &ut, string functionName, bool result);

void write_result_direct(unit_test &ut, string functionName, bool is_first);

void write_result_indirect(unit_test &ut, string functionName, bool is_first);

void write_test_summary(unit_test &ut, cradle::time endTime);

}


#endif // CRADLE_UNIT_TESTS_HPP