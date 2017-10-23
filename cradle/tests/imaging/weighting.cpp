#include <cradle/imaging/weighting.hpp>
#include <cradle/common.hpp>

#define BOOST_TEST_MODULE imaging
#include <cradle/imaging/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(weighted_combination_test)
{
    unsigned const s = 3;

    double data1[s * s] = {
        0, 0, 0,
        0,10, 0,
        0, 0, 0,
    };
    image<2,double,const_view> src1 =
        make_const_view(data1, make_vector(s, s));

    double data2[s * s] = {
        2, 2, 0,
        0, 0, 6,
        0, 0, 8,
    };
    image<2,double,const_view> src2 =
        make_const_view(data2, make_vector(s, s));

    {
        weighted_image<2,double,double,const_view> wis[2] = {
            { src1, 0.5 }, { src2, 0.5 } };

        image<2,double,weighted_combination<double,const_view> > combo =
            make_weighted_combination(wis, 2);

        double results[] = {
            1, 1, 0,
            0, 5, 3,
            0, 0, 4,
        };
        CRADLE_CHECK_IMAGE(combo, results, results + s * s);
    }

    double data3[s * s] = {
        0, 2, 1,
        0, 2, 1,
       10, 4, 1,
    };
    image<2,double,const_view> src3 =
        make_const_view(data3, make_vector(s, s));

    {
        weighted_image<2,double,double,const_view> wis[3] = {
            { src1, 0.2 }, { src2, 0.5 }, { src3, 0.25 } };

        image<2,double,weighted_combination<double,const_view> > combo =
            make_weighted_combination(wis, 3);

        double results[] = {
            1,  1.5, 0.25,
            0,  2.5, 3.25,
          2.5,    1, 4.25,
        };
        CRADLE_CHECK_IMAGE(combo, results, results + s * s);
    }
}

BOOST_AUTO_TEST_CASE(integer_test)
{
    unsigned const s = 3;

    int data1[s * s] = {
        0, 0, 0,
        0,10, 0,
        0, 0, 0,
    };
    image<2,int,const_view> src1 =
        make_const_view(data1, make_vector(s, s));

    int data2[s * s] = {
        2, 2, 0,
        0, 0, 6,
        0, 0, 8,
    };
    image<2,int,const_view> src2 =
        make_const_view(data2, make_vector(s, s));

    {
        weighted_image<2,double,int,const_view> wis[2] = {
            { src1, 0.5 }, { src2, 0.5 } };

        image<2,double,weighted_combination<int,const_view> > combo =
            make_weighted_combination(wis, 2);

        double results[] = {
            1, 1, 0,
            0, 5, 3,
            0, 0, 4,
        };
        CRADLE_CHECK_IMAGE(combo, results, results + s * s);
    }
}
