#include <cradle/memory_cache.hpp>

#define BOOST_TEST_MODULE memory_cache
#include <cradle/test.hpp>

using namespace cradle;

struct set_int_job : composable_background_job
{
    set_int_job() {}
    set_int_job(memory_cache& cache, alia::id_interface const& id, int value)
      : cache_(&cache), value_(value)
    {
        id_.store(id);
    }
    void execute(
        check_in_interface& check_in,
        progress_reporter_interface& reporter)
    {
        check_in();
        reporter(0);
        set_cached_data(*cache_, id_.get(), make_immutable(value_));
        reporter(1);
    }
    memory_cache* cache_;
    alia::owned_id id_;
    int value_;
};

BOOST_AUTO_TEST_CASE(simple_caching_test)
{
    using alia::make_id;

    memory_cache cache;

    cached_data_ptr<int> p;
    BOOST_CHECK(!p.is_initialized());
    BOOST_CHECK(!p.is_ready());
    BOOST_CHECK(p.is_nowhere());

    p.reset(cache, make_id(0));
    BOOST_CHECK(p.is_initialized());
    BOOST_CHECK(!p.is_ready());
    BOOST_CHECK(p.is_nowhere());
    BOOST_CHECK(p.key() == make_id(0));

    cached_data_ptr<int> q(cache, make_id(1));
    BOOST_CHECK(q.is_initialized());
    BOOST_CHECK(!q.is_ready());
    BOOST_CHECK(q.is_nowhere());
    BOOST_CHECK(q.key() == make_id(1));

    p = q;
    BOOST_CHECK(p.is_initialized());
    BOOST_CHECK(!p.is_ready());
    BOOST_CHECK(p.is_nowhere());
    BOOST_CHECK(p.key() == make_id(1));

    set_cached_data(cache, make_id(1), make_immutable(12));

    BOOST_CHECK(!p.is_ready());
    BOOST_CHECK(p.is_nowhere());
    p.update();
    BOOST_CHECK(p.is_ready());
    BOOST_CHECK(!p.is_nowhere());
    BOOST_CHECK_EQUAL(*p, 12);

    BOOST_CHECK(!q.is_ready());
    BOOST_CHECK(q.is_nowhere());
    q.update();
    BOOST_CHECK(q.is_ready());
    BOOST_CHECK(!q.is_nowhere());
    BOOST_CHECK_EQUAL(*q, 12);

    background_execution_system bg;

    p.reset(cache, make_id(0));
    BOOST_CHECK(!p.is_ready());
    BOOST_CHECK(p.is_nowhere());

    background_job_interface job;
    add_background_job(bg, &job, new set_int_job(cache, make_id(0), 4));
    p.set_job(&job);

    p.update();
    while (!p.is_ready())
    {
        BOOST_CHECK(p.state() == cached_data_state::COMPUTING);
        p.update();
    }

    BOOST_CHECK(!p.is_nowhere());
    BOOST_CHECK_EQUAL(*p, 4);
}
