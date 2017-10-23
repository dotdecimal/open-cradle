#ifndef CRADLE_RT_DOSE_HPP
#define CRADLE_RT_DOSE_HPP

#include <cradle/common.hpp>
#include <cradle/rt/common.hpp>
#include <cradle/rt/plan.hpp>


namespace cradle {

// Stores image slice dicom data in 2D or 3D form.
api(struct with(N :2, 3))
template<unsigned N>
struct rt_image_data
{
    // Image data (counts, pixels, spacing, position, are stored implicitly)
    image<N, double, shared> img_data;
    // Instance number
    int instance_number;
    // Direction cosines of the first row and the first column of data with respect to the
    // patient
    std::vector<double> image_orientation;
    // Intended interpretation of the pixel data (e.g. monochrome1 or monochrome2,
    // [palette color, rgb etc, are not supported])
    string photometric_interpretation;
};

// Type of dose summation file
api(enum)
enum class dose_summation_type
{
    // Dose calculated for entire delivery of all fraction groups of the RT Plan
    PLAN,
    // Dose calculated for entire delivery of a single fraction group of the RT Plan
    FRACTION,
    // Dose calculated for entire delivery of one or more beams of the RT Plan
    BEAM
};

void static inline ensure_default_initialization(dose_summation_type &v)
{
    v = dose_summation_type::PLAN;
}

#ifdef ERROR
#undef ERROR
#endif // ERROR

// Type of dose
api(enum)
    enum class dose_type
{
    // Physical dose
    PHYSICAL,
    // Physical dose after correction for biological effect using modeling technique
    EFFECTIVE,
    // Difference between desired and planned dose
    ERROR
};

void static inline ensure_default_initialization(dose_type &v)
{
    v = dose_type::PHYSICAL;
}

// Radiotherapy dose data (DICOM-like)
api(struct)
struct rt_dose : dicom_file
{
    // RT Series information
    dicom_rt_series series;
    // CT Image slice data
    rt_image_data<3> dose;
    // The type of dose
    dose_type type;
    // The type of dose summation
    dose_summation_type summation_type;
    // Referenced RT Plan sequence
    ref_dicom_item ref_plan_data;
    // Must be set to FG index if summation type is Fraction or Beam
    int ref_fraction_number;
    // Must be set to beam index if summation type is Beam
    int ref_beam_number;
};

void static inline ensure_default_initialization(rt_dose &v)
{
    v.ref_fraction_number = -1;
    v.ref_beam_number = -1;
}


}

#endif
