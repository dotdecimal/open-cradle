#ifndef CRADLE_RT_TYPES_HPP
#define CRADLE_RT_TYPES_HPP

#include <cradle/api.hpp>
#include <cradle/common.hpp>
#include <cradle/rt/common.hpp>
#include <cradle/rt/ct_image.hpp>
#include <cradle/rt/dose.hpp>
#include <cradle/rt/plan.hpp>
#include <cradle/rt/structure_set.hpp>


namespace cradle {

// Contains a complete set of rt information for study (treatment course)
api(struct)
struct rt_study : dicom_study
{
    // Plan
    rt_ion_plan plan;
    // Structure set
    rt_structure_set structure_set;
    // Full ct image with all slices
    ct_image ct;
    // Dose
    std::vector<rt_dose> doses;
};

// Union of the different data types that a dicom file could represent
api(union)
union dicom_object
{
    // Structure set
    rt_structure_set structure_set;
    // Full ct image with all slices
    ct_image ct_image;
    // Dose
    rt_dose dose;
    // Plan
    rt_ion_plan plan;
};

// Vector of rt study data that makes up a patient record
api(struct)
struct dicom_patient
{
    // List of Studies for the patient
    std::vector<rt_study> studies;
};



}

#endif
