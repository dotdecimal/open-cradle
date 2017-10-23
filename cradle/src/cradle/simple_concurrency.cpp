#include <cradle/simple_concurrency.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

namespace cradle {

void lower_thread_priority(boost::thread& thread);

// This class is used for dynamically assigning objects to threads.
// You tell it how many objects there are and have your threads call it
// repeatedly to get objects.
struct dynamic_thread_object_assigner
{
    dynamic_thread_object_assigner(size_t n_objects);
    // Call this to get an object to process.
    // If there is one available, this returns true and assigns *object_index
    // to the index of the object to process.
    // If it returns false, all objects have already been assigned.
    bool get_object(size_t* object_index);
    size_t total_objects() const { return n_objects_; }
 private:
    size_t n_objects_, next_object_;
    boost::mutex mutex_;
};

dynamic_thread_object_assigner::dynamic_thread_object_assigner(
    size_t n_objects)
 : n_objects_(n_objects)
 , next_object_(0)
{}

bool dynamic_thread_object_assigner::get_object(size_t* object_index)
{
    boost::mutex::scoped_lock lock(mutex_);
    if (next_object_ < n_objects_)
    {
        *object_index = next_object_;
        ++next_object_;
        return true;
    }
    else
        return false;
}

struct shared_progress_reporting_state
{
    shared_progress_reporting_state(
        progress_reporter_interface* reporter, size_t n_jobs)
     : reporter(reporter)
     , n_jobs(n_jobs)
     , total_progress(0)
    {}

    progress_reporter_interface* reporter;
    size_t n_jobs;
    float total_progress;
    boost::mutex mutex;
};

struct worker_thread_progress_reporter : progress_reporter_interface
{
    worker_thread_progress_reporter(shared_progress_reporting_state* state)
    {
        state_ = state;
        progress_so_far_ = 0;
    }

    void operator()(float progress)
    {
        boost::lock_guard<boost::mutex> lock(state_->mutex);
        state_->total_progress += (progress - progress_so_far_) /
            state_->n_jobs;
        progress_so_far_ = progress;
        (*state_->reporter)(state_->total_progress);
    }

 private:
    shared_progress_reporting_state* state_;
    // progress so far for this job
    float progress_so_far_;
};

struct shared_check_in_state
{
    shared_check_in_state(check_in_interface* check_in)
      : check_in(check_in)
      , abort(false)
    {}
    check_in_interface* check_in;
    boost::mutex mutex;
    bool abort;
};

struct worker_thread_aborted : exception
{
    worker_thread_aborted() : exception("aborted") {}
};

struct worker_thread_check_in : check_in_interface
{
    worker_thread_check_in(shared_check_in_state* state)
      : state_(state)
    {}

    void operator()()
    {
        if (state_->abort)
            throw worker_thread_aborted();
        try
        {
            boost::lock_guard<boost::mutex> lock(state_->mutex);
            (*state_->check_in)();
        }
        catch(...)
        {
            throw worker_thread_aborted();
        }
    }

  private:
    shared_check_in_state* state_;
};

enum class worker_thread_result
{
    SUCCEEDED,
    FAILED,
    ABORTED,
};

struct worker_thread_error_report
{
    worker_thread_result result;
    string msg;
};

struct worker_thread
{
    worker_thread(
        dynamic_thread_object_assigner* assigner,
        simple_job_interface** jobs,
        shared_progress_reporting_state* reporter_state,
        shared_check_in_state* check_in_state,
        worker_thread_error_report* report)
      : assigner_(assigner)
      , jobs_(jobs)
      , reporter_state_(reporter_state)
      , check_in_state_(check_in_state)
      , report_(report)
    {}

    void operator()()
    {
        try
        {
            while (1)
            {
                size_t i;
                if (!assigner_->get_object(&i))
                    break;
                worker_thread_check_in check_in(check_in_state_);
                worker_thread_progress_reporter reporter(reporter_state_);
                jobs_[i]->execute(check_in, reporter);
            }
            report_->result = worker_thread_result::SUCCEEDED;
        }
        catch (worker_thread_aborted const&)
        {
            report_->result = worker_thread_result::ABORTED;
        }
        catch (std::bad_alloc const&)
        {
            check_in_state_->abort = true;
            report_->result = worker_thread_result::FAILED;
            report_->msg = "out of memory";
        }
        catch (std::exception const& e)
        {
            check_in_state_->abort = true;
            report_->result = worker_thread_result::FAILED;
            report_->msg = e.what();
        }
        catch (...)
        {
            check_in_state_->abort = true;
            report_->result = worker_thread_result::FAILED;
            report_->msg = "unknown error";
        }
    }

 private:
    dynamic_thread_object_assigner* assigner_;
    simple_job_interface** jobs_;
    shared_progress_reporting_state* reporter_state_;
    shared_check_in_state* check_in_state_;
    worker_thread_error_report* report_;
};

void execute_jobs_concurrently(
    check_in_interface& check_in,
    progress_reporter_interface& reporter,
    size_t n_jobs, simple_job_interface** jobs)
{
    shared_progress_reporting_state reporter_state(&reporter, n_jobs);
    shared_check_in_state check_in_state(&check_in);

    dynamic_thread_object_assigner assigner(n_jobs);

    size_t n_threads_to_create = (std::min)(
        size_t(boost::thread::hardware_concurrency()),
        n_jobs);

    //std::cout << "Creating Threads: " << n_threads_to_create << std::endl;

    std::vector<alia__shared_ptr<boost::thread> > threads;
    threads.reserve(n_threads_to_create);

    std::vector<worker_thread_error_report> error_reports(
        n_threads_to_create);

    for (size_t i = 0; i != n_threads_to_create; ++i)
    {
        worker_thread fn(
            &assigner, jobs, &reporter_state,
            &check_in_state, &error_reports[i]);
        threads.push_back(alia__shared_ptr<boost::thread>(
            new boost::thread(fn)));
        lower_thread_priority(*threads.back());
    }

    // Wait for all threads to finish.
    for (size_t i = 0; i != n_threads_to_create; ++i)
        threads[i]->join();

    // The idea here is that if one of the worker threads was aborted by
    // check_in, calling it again here should abort the entire calculation,
    // giving the same behavior that you'd get from a single-threaded
    // calculation.
    check_in();

    // Check if any threads failed or were aborted.
    bool aborted = false;
    for (size_t i = 0; i != n_threads_to_create; ++i)
    {
        switch (error_reports[i].result)
        {
         case worker_thread_result::FAILED:
            throw worker_thread_failed(error_reports[i].msg);
         case worker_thread_result::ABORTED:
            aborted = true;
            break;
         default:
             break;
        }
    }
    // This shouldn't happen, but just in case...
    if (aborted)
        throw worker_thread_failed("aborted");

    check_in();
}

}
