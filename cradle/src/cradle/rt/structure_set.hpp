#ifndef CRADLE_RT_STRUCTURE_SET_HPP
#define CRADLE_RT_STRUCTURE_SET_HPP

#include <cradle/api.hpp>
#include <cradle/common.hpp>
#include <cradle/geometry/polygonal.hpp>
#include <cradle/rt/common.hpp>

namespace cradle {

// Dicom Structure Type enum.
api(enum)
enum class rt_structure_type
{
    // point
    POINT,
    // polygon
    CLOSED_PLANAR
};

// Dicom ROI Type enum.
api(enum)
enum class
rt_roi_type
{
    // See DICOM PS3.3 C.8.8.8.1
    EXTERNAL,
    PTV,
    CTV,
    GTV,
    TREATED_VOLUME,
    IRRAD_VOLUME,
    BOLUS,
    AVOIDANCE,
    ORGAN,
    MARKER,
    REGISTRATION,
    ISOCENTER,
    CONTRAST_AGENT,
    CAVITY,
    SUPPORT,
    FIXATION,
    DOSE_REGION,
    CONTROL
};

// Stores structure slice data.
api(struct)
struct rt_contour
{
    // position of the slice in the image along the slice axis
    double position;
    // polyset of the structure on the slice
    polyset region;
};

typedef std::vector<rt_contour> rt_contour_list;

// Dicom structure geometry union for either a point or a list of contours.
api(union)
union rt_roi_geometry
{
    // point
    vector3d point;
    // slice list for closed planar structures
    rt_contour_list slices;
};

// Stores data for a dicom structure.
api(struct)
struct rt_structure
{
    // individual structure name
    string name;
    // individual structure label
    string description;
    // the structure number is unique within the structure's parent set
    int number;
    // referenced frame of reference sequence
    string ref_frame_of_reference_uid;
    // set color of structure
    rgb8 color;
    // geometry of the item (e.g., point, closed planar structure)
    rt_roi_geometry geometry;
    // ROI type for the structure
    optional<rt_roi_type> roi_type;
};

api(union)
union rt_structure_list
{
    std::vector<rt_structure> structure_list;
    std::vector<object_reference<rt_structure> > ref_structure_list;
};

// Holds dicom structure set IOD information.
api(struct)
struct rt_structure_set : dicom_file
{
    // RT Series information
    dicom_rt_series series;
    // structure set label
    string label;
    // structure set Name
    string name;
    // structure set description
    string description;
    // referenced CT image series uid
    string ref_image_series_uid;
    // list of all structures
    rt_structure_list structures;
    // approval status information
    rt_approval approval_info;
    // sequence of Class and Instance IDs for relating structures to ct images
    std::vector<ref_dicom_item> contour_image_sequence;
};

}

#endif
