#include <cradle/api.hpp>

namespace cradle {

void release_cache_record(background_cache_record* record);

template<class T>
void background_data_ptr<T>::reset()
{
    untyped_.reset();
    data_ = 0;
}

template<class T>
void background_data_ptr<T>::update()
{
    if (this->state() != background_data_state::READY)
    {
        untyped_.update();
        this->refresh_typed();
    }
}

template<class T>
void background_data_ptr<T>::swap(background_data_ptr& other)
{
    using std::swap;
    swap(untyped_, other.untyped_);
    swap(data_, other.data_);
}

template<class T>
void background_data_ptr<T>::refresh_typed()
{
    if (this->state() == background_data_state::READY)
        cast_immutable_value(&data_, untyped_.data().ptr.get());
    else
        data_ = 0;
}

void set_cache_record_job(background_cache_record* record,
    background_job_controller* job);

template<class Result>
void add_background_job(
    background_data_ptr<Result>& ptr, background_execution_system& system,
    background_job_queue_type queue, background_job_interface* job,
    background_job_flag_set flags, int priority)
{
    add_untyped_background_job(ptr.untyped(), system, queue, job, flags,
        priority);
    ptr.refresh_typed();
}

template<class T>
void swap_in_cached_data(
    background_execution_system& system, id_interface const& key, T& value)
{
    object<T> tmp;
    swap_in(tmp, value);
    set_cached_data(system, key, erase_type(tmp));
}

}
