#include <cradle/rt/utilities.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace cradle {

person_name parse_dicom_name(string const& dicom)
{
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> separators("^", "", boost::keep_empty_tokens);
    string trimmed_name = boost::trim_right_copy(dicom);
    tokenizer tokens(trimmed_name, separators);
    tokenizer::iterator i = tokens.begin();
    person_name pn;
    if (i != tokens.end())
        pn.family_name = *i++;
    if (i != tokens.end())
        pn.given_name = *i++;
    if (i != tokens.end())
        pn.middle_name = *i++;
    if (i != tokens.end())
        pn.prefix = *i++;
    if (i != tokens.end())
        pn.suffix = *i++;
    return pn;
}

string get_dicom_representation(person_name const& pn)
{
    return pn.family_name + "^" + pn.given_name + "^" + pn.middle_name + "^" +
        pn.prefix + "^" + pn.suffix;
}

static string
make_full_name(
    string const& family_name,
    string const& given_name,
    string const& middle_name,
    string const& prefix,
    string const& suffix)
{
    string name;

    name += prefix;

    #define APPEND(str) \
        if (str != "") \
        { \
            if (name != "") \
                name += " "; \
            name += str; \
        }
    APPEND(given_name);
    APPEND(middle_name);
    APPEND(family_name);

    if (suffix != "")
        name += ", " + suffix;

    return name;
}

string get_full_name(person_name const& pn)
{
    return make_full_name(pn.family_name, pn.given_name, pn.middle_name,
        pn.prefix, pn.suffix);
}

vector3d
transform_scanner_to_patient(vector3d const& p, patient_position_type pos)
{
    switch(pos)
    {
        case patient_position_type::HFS:
            return p;
        case patient_position_type::HFP:
            return make_vector(-p[0], -p[1], p[2]);
        case patient_position_type::FFS:
            return make_vector(-p[0], p[1], -p[2]);
        case patient_position_type::FFP:
            return make_vector(p[0], -p[1], -p[2]);
        case patient_position_type::HFDR:
            return make_vector(-p[1], p[0], p[2]);
        case patient_position_type::HFDL:
            return make_vector(p[1], -p[0], p[2]);
        case patient_position_type::FFDR:
            return make_vector(-p[1], -p[0], -p[2]);
        case patient_position_type::FFDL:
            return make_vector(p[1], p[0], -p[2]);
        default:
            return p;
    }
}

void
set_ct_spatial_mapping(
    image<2, double, cradle::shared> &ct_img,
    ct_image_slice const& img_slice)
{
    auto sl = img_slice.slice;
    assert(ct_img.size[0] == sl.content.cols && ct_img.size[1] == sl.content.rows);
    ct_img.axes[0][0] = sl.pixel_spacing[1] * sl.image_orientation[0];
    ct_img.axes[0][1] = sl.pixel_spacing[0] * sl.image_orientation[1];
    ct_img.axes[1][0] = sl.pixel_spacing[1] * sl.image_orientation[3];
    ct_img.axes[1][1] = sl.pixel_spacing[0] * sl.image_orientation[4];
    // Set origin of cradle image as corner of pixel, from dicom pixel center
    auto pixel_center = slice(sl.image_position, 2);
    ct_img.origin = pixel_center - 0.5 * ct_img.axes[0] - 0.5 * ct_img.axes[1];
}

void
standardize_ct_spatial_mapping(
    image<2, double, cradle::shared> &ct_img,
    ct_image_slice const& img_slice)
{
    set_ct_spatial_mapping(ct_img, img_slice);
    auto img = aligned_view(ct_img);
    ct_img.origin = img.origin;
    ct_img.axes = img.axes;
    ct_img.size = img.size;
    ct_img.step = make_vector<ptrdiff_t>(1, img.size[0]);
}

cradle::structure_geometry
get_geometry_from_structure(
    rt_contour_list const& slice_list,
    slice_description_list const& master_slices)
{
    cradle::structure_geometry sg;
    sg.master_slice_list = master_slices;

    size_t i = 0;
    for (auto const& s : slice_list)
    {
        bool found = false;
        for (i = 0; i < master_slices.size(); ++i)
        {
            if (std::fabs(s.position - master_slices[i].position) <= 0.1)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            throw exception("Master slices do not match given structure slices");
        }

        sg.slices[master_slices[i].position] = s.region;
    }

    return sg;
}

slice_description_list
get_ct_image_slice_descriptions(dicom_object const& dicom_ct_object)
{
    if (dicom_ct_object.type != dicom_object_type::CT_IMAGE)
    {
        throw exception("DICOM Type is not CT Image");
    }

    return get_slices_for_ct_image(as_ct_image(dicom_ct_object));
}

slice_description_list
get_slices_for_ct_image(ct_image const& ct)
{
    if (ct.type == ct_image_type::IMAGE_SET)
    {
        auto grid = get_grid(as_image_set(ct).image);
        return get_slices_for_grid(grid, 2);
    }
    else if (ct.type == ct_image_type::IMAGE_SLICES)
    {
        auto slice_list =
            map(
                [](ct_image_slice const& slice)
        {
            return slice_description(slice.slice.position, slice.slice.thickness);
        },
                as_image_slices(ct));

        std::sort(begin(slice_list), end(slice_list));
        return slice_list;
    }

    throw exception("Unsupported CT Image type");
}

rt_ref_beam
make_rt_ref_beam(
    double beam_dose,
    unsigned beam_number,
    double final_meterset_weight,
    vector3d const& ref_point)
{
    return rt_ref_beam(beam_number, ref_point, beam_dose, final_meterset_weight);
}

rt_fraction
make_rt_fraction(
    unsigned fg_index,
    unsigned number_of_fractions,
    std::vector<rt_ref_beam> const& ref_beams,
    string const& description,
    unsigned fraction_cycle_length,
    unsigned fractions_per_day,
    string const& fraction_pattern)
{
    return
        rt_fraction(
            fg_index, number_of_fractions, ref_beams, description,
            fractions_per_day, fraction_cycle_length, fraction_pattern);
}

rt_ion_plan
add_fraction_groups_to_plan(
    rt_ion_plan const& plan,
    std::vector<rt_fraction> const& fraction_groups,
    std::vector<rt_ion_beam> const& beams)
{
    rt_ion_plan result(plan);
    result.fractions.insert(result.fractions.end(),
        fraction_groups.begin(), fraction_groups.end());
    result.beams.insert(result.beams.end(),
        beams.begin(), beams.end());
    return result;
}

string
format_dicom_modality(dicom_modality const& modality)
{
    switch (modality)
    {
        case dicom_modality::CT:
            return "CT Image Set";
        case dicom_modality::RTDOSE:
            return "Dose";
        case dicom_modality::RTPLAN:
            return "Plan";
        case dicom_modality::RTSTRUCT:
            return "Structure Set";
        default:
            return "Error";
    }
}

string
format_patient_sex(patient_sex sex)
{
    switch (sex)
    {
        case patient_sex::F:
            return "Female";
        case patient_sex::M:
            return "Male";
        case patient_sex::O:
            return "Other";
        default:
            assert(0);
            return "";
    }
}

void copy_dicom_file_data(const dicom_file *src, dicom_file *dst)
{
    dst->patient_data = src->patient_data;
    dst->study_data = src->study_data;
    dst->equipment_data = src->equipment_data;
    dst->frame_of_ref_uid = src->frame_of_ref_uid;
    dst->sop_data = src->sop_data;
}

}