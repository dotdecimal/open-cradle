#ifndef CRADLE_RT_UTILITIES_HPP
#define CRADLE_RT_UTILITIES_HPP

#include <cradle/rt/types.hpp>

namespace cradle {

// Parse a string in DICOM's PN format into a person_name.
person_name parse_dicom_name(string const& dicom);

// Get the DICOM PN representation of a person_name.
string get_dicom_representation(person_name const& pn);

// Get the standard textual representation of a person's full name.
string get_full_name(person_name const& pn);

// Returns a point transformed from scanner space to patient space.
api(fun)
// Point transformed according to patient position
vector3d transform_scanner_to_patient(
    // Point to be transformed
    vector3d const& p,
    // Patient position which will determine the transform
    patient_position_type pos);

// Sets a 2D image spatial mapping based on the ct image slice data.
// This function computes the origin and axes based on the slice position and image_orientation data.
void set_ct_spatial_mapping(image<2, double, cradle::shared> &ct_img, ct_image_slice const& img_slice);

// Sets a 2D image spatial mapping based on the ct image slice data.
// This function computes the origin and axes such that the image is positioned in "standard" image notation,
// with the origin at the most negative corner of the image.
// Note: This function assumes the image pixel data is already in standard order, as such the image step is
// reset to standard values (1, size[0]).
void standardize_ct_spatial_mapping(image<2, double, cradle::shared> &ct_img, ct_image_slice const& img_slice);

// Takes in a list of contours, generates a structure_geometry from that and returns
// the structure_geometry
api(fun)
// Structure geometry that was generated from slices
cradle::structure_geometry
get_geometry_from_structure(
    // list of slices used to generate structure_geometry
    rt_contour_list const& dicom_slice_list,
    // list of all slices that the structure could have a contour on.
    slice_description_list const& master_slices);

// Gets the slice list from a dicom object ct image.
api(fun)
// slice_description_list
slice_description_list
get_ct_image_slice_descriptions(
    // Dicom object of CT image
    dicom_object const& dicom_ct_object);

slice_description_list
get_slices_for_ct_image(ct_image const& ct);

// Makes a rt ref beam from provided data.
api(fun)
// Generated rt ref beam
rt_ref_beam
make_rt_ref_beam(
    // beam dose
    double beam_dose,
    // beam number
    unsigned beam_number,
    // meterset weight of beam
    double final_meterset_weight,
    // beam isocenter
    vector3d const& isocenter);

// Makes a fraction group with the provided beams.
api(fun)
// fraction group
rt_fraction
make_rt_fraction(
    // fraction group index
    unsigned fg_index,
    // number of fractions in fraction group
    unsigned number_of_fractions,
    // beams in the fraction group
    std::vector<rt_ref_beam> const& ref_beams,
    // description for fraction group
    string const& description,
    // length of fraction pattern units(weeks)
    unsigned fraction_cycle_length,
    // number of treatment sessions per day (typically 1)
    unsigned fractions_per_day,
    // binary string describing the fraction daily delivery pattern (length of this string
    // should be (fractions_per_day x fraction_pattern_length x 7)
    string const& fraction_pattern);

// Generates a rt ion plan from provided fraction groups and beams.
api(fun)
// rt ion plan
rt_ion_plan
add_fraction_groups_to_plan(
    // base plan
    rt_ion_plan const& plan,
    // fraction groups to include in plan
    std::vector<rt_fraction> const& fraction_groups,
    // beams to include in plan
    std::vector<rt_ion_beam> const& beams);

// Format the dicom_modality enum into a non capitalized string
string
format_dicom_modality(dicom_modality const& modality);

string
format_patient_sex(patient_sex sex);

// Copies the dicom_file base information from src to dst
void copy_dicom_file_data(const dicom_file *src, dicom_file *dst);

}

#endif
