#include <cradle/pooled_list.hpp>
#include <vector>
#include <boost/random.hpp>

#define BOOST_TEST_MODULE pooled_list
#include <cradle/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(pooled_list_test)
{
    std::vector<double> v;
    pooled_list<double> l;

    BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), l.begin(), l.end());

    typedef boost::minstd_rand base_generator_type;

    base_generator_type generator(42u);

    boost::uniform_real<> uni_dist(0,1);
    boost::variate_generator<base_generator_type&, boost::uniform_real<> > uni(
        generator, uni_dist);

    for (unsigned i = 0; i < 2 * l.block_size + 1; ++i)
    {
        double d = uni();
        v.push_back(d);
        *l.alloc() = d;
        BOOST_CHECK_EQUAL_COLLECTIONS(v.begin(), v.end(), l.begin(), l.end());
    }
}
