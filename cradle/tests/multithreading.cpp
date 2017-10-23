#include <cradle/simple_concurrency.hpp>
#include <cradle/background/system.hpp>
#include <cradle/background/api.hpp>

#define BOOST_TEST_MODULE cradle_multithreading
#include <cradle/test.hpp>

using namespace cradle;

struct simple_set_int_job : simple_job_interface
{
    simple_set_int_job() {}
    simple_set_int_job(int* n, int value)
      : n_(n), value_(value)
    {}
    void execute(
        check_in_interface& check_in,
        progress_reporter_interface& reporter)
    {
        check_in();
        reporter(0);
        *n_ = value_;
        reporter(1);
    }
    int* n_;
    int value_;
};

struct progress_recorder : progress_reporter_interface
{
    progress_recorder(float* storage)
      : storage_(storage)
    {}
    void operator()(float progress)
    { *storage_ = progress; }
    float* storage_;
};

BOOST_AUTO_TEST_CASE(execute_jobs_concurrently_test)
{
    int const n_jobs = 1000;

    int n[n_jobs];
    std::vector<simple_set_int_job> jobs;
    for (int i = 0; i != n_jobs; ++i)
        jobs.push_back(simple_set_int_job(n + i, i));

    float progress;
    progress_recorder reporter(&progress);

    null_check_in check_in;

    execute_jobs_concurrently(check_in, reporter, n_jobs, &jobs[0]);
    for (int i = 0; i != n_jobs; ++i)
        BOOST_CHECK_EQUAL(n[i], i);
    CRADLE_CHECK_ALMOST_EQUAL(progress, 1.f);

    progress = 0;
    for (int i = 0; i != n_jobs; ++i)
        n[i] = -1;

    execute_jobs_concurrently(check_in, reporter, 6, &jobs[0]);
    for (int i = 0; i != 6; ++i)
        BOOST_CHECK_EQUAL(n[i], i);
    CRADLE_CHECK_ALMOST_EQUAL(progress, 1.f);
}

struct composable_set_int_job : background_job_interface
{
    composable_set_int_job() {}
    composable_set_int_job(int* n, int value)
      : n_(n), value_(value)
    {}
    void execute(
        check_in_interface& check_in,
        progress_reporter_interface& reporter)
    {
        check_in();
        reporter(0);
        *n_ = value_;
        reporter(1);
    }
    int* n_;
    int value_;
};

BOOST_AUTO_TEST_CASE(background_execution_system_test)
{
    background_execution_system bg;

    int const n_jobs = 1000;

    int n[n_jobs];
    std::vector<alia__shared_ptr<background_job_controller> > jobs(n_jobs);
    for (int i = 0; i != n_jobs; ++i)
    {
        jobs[i].reset(new background_job_controller);
        add_background_job(bg, &*jobs[i],
            new composable_set_int_job(n + i, i));
    }

    for (int i = 0; i != n_jobs; ++i)
    {
        while (jobs[i]->state() != background_job_state::FINISHED)
            ;
        CRADLE_CHECK_ALMOST_EQUAL(jobs[i]->progress(), 1.f);
        BOOST_CHECK_EQUAL(n[i], i);
    }
}
