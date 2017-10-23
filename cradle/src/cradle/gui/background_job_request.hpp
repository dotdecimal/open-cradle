#ifndef CRADLE_GUI_BACKGROUND_JOB_REQUEST_HPP
#define CRADLE_GUI_BACKGROUND_JOB_REQUEST_HPP

#include <cradle/background/api.hpp>
#include <cradle/gui/common.hpp>
#include <cradle/gui/app/interface.hpp>
//#include <cradle/io/web_io.hpp>

#include <boost/function.hpp>

namespace cradle {

api(struct internal)
struct background_job_result
{
    string message;
    bool error;
};

struct background_job_data
{
    untyped_background_data_ptr ptr;
    local_identity abbreviated_identity;
};


bool update_general_background_job(
    gui_context& ctx, background_job_data& data,
    id_interface const& id,
    boost::function<background_job_interface*()> const& create_background_job);


struct typed_background_job_data
{
    background_job_data untyped;
    background_job_result const* result;
    typed_background_job_data() : result(0) {}
};

struct background_job_accessor : accessor<background_job_result>
{
    background_job_accessor() {}
    background_job_accessor(typed_background_job_data& data)
      : data_(&data)
    {}
    id_interface const& id() const
    {
        if (data_->untyped.ptr.is_initialized())
        {
            id_ = get_id(data_->untyped.abbreviated_identity);
            return id_;
        }
        else
            return no_id;
    }
    background_job_result const& get() const { return *data_->result; }
    bool is_gettable() const { return data_->result != 0; }
    bool is_settable() const { return false; }
    void set(background_job_result const& value) const {}
 private:
    typed_background_job_data* data_;
    mutable value_id_by_reference<local_id> id_;
};




}

#endif