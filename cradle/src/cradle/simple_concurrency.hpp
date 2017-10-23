#ifndef CRADLE_SIMPLE_CONCURRENCY_HPP
#define CRADLE_SIMPLE_CONCURRENCY_HPP

#include <cradle/common.hpp>

namespace cradle {

// This is the interface required of jobs that are to be executed using the
// simple concurrency facilities.
struct simple_job_interface
{
    virtual ~simple_job_interface() {}

    virtual void execute(
        check_in_interface& check_in,
        progress_reporter_interface& reporter) = 0;
};

// Given a list of jobs to be done, execute_jobs_concurrently will spawn an
// appropriate number of threads and dynamically allocate the jobs to the
// threads so that they can be done in parallel. The number of threads is
// chosen based on the number of jobs to be done and the number of processor
// cores in the system.
//
// Each job is invoked as follows...
//
// jobs[i].execute(job_check_in, job_reporter);
//
// .. where job_reporter is created from the supplied reporter under the
// assumption that each job represents a roughly equal amount of work,
// and where job_check_in is created to protect the supplied check_in against
// simultaneous access by multiple jobs.
//
// If an exception is thrown by any job, it records the message associated
// with the exception, aborts all the other jobs, and throws a new
// worker_thread_failed exception with the same message (the original type
// of the exception is lost).

struct worker_thread_failed : exception
{
    worker_thread_failed(string const& msg)
      : exception(msg)
    {}
    ~worker_thread_failed() throw() {}
};

void execute_jobs_concurrently(
    check_in_interface& check_in,
    progress_reporter_interface& reporter,
    size_t n_jobs, simple_job_interface** jobs);

template<class Job>
void execute_jobs_concurrently(
    check_in_interface& check_in,
    progress_reporter_interface& reporter,
    size_t n_jobs, Job* jobs)
{
    std::vector<simple_job_interface*> job_ptrs(n_jobs);
    for (size_t i = 0; i != n_jobs; ++i)
        job_ptrs[i] = &jobs[i];
    execute_jobs_concurrently(check_in, reporter, n_jobs, &job_ptrs[0]);
}

}

#endif
