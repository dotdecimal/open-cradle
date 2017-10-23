#ifndef CRADLE_RT_COMMON_HPP
#define CRADLE_RT_COMMON_HPP

#include <cradle/imaging/color.hpp>
#include <cradle/imaging/slicing.hpp>
#include <cradle/date_time.hpp>
#include <string>
#include <vector>

typedef unsigned short UINT16;

namespace cradle {


// Type of dicom file
api(enum)
enum class dicom_modality
{
    // Plan
    RTPLAN,
    // Structure Set
    RTSTRUCT,
    // CT Image
    CT,
    // Dose
    RTDOSE
};

void static inline ensure_default_initialization(dicom_modality &v)
{
    v = dicom_modality::RTPLAN;
}

api(struct)
struct ref_dicom_item
{
    // class uid
    string ref_class_uid;
    // instance uid
    string ref_instance_uid;
};

// Stores a person name, according to DICOM specifications
api(struct)
struct person_name
{
    // Patient last name
    string family_name;
    // Patient first name
    string given_name;
    // Patient middle name
    string middle_name;
    // Patient prefix (mr, mrs, etc)
    string prefix;
    // Patient suffix (jr, sr etc)
    string suffix;
};

// Patient sex
api(enum)
enum class patient_sex
{
    // male
    M,
    // female
    F,
    // other
    O
};

void static inline ensure_default_initialization(patient_sex &ps)
{
    ps = patient_sex::O;
}

// Stores general patient information (DICOM patient module).
api(struct)
struct patient
{
    // patient's name
    person_name name;
    // Id (MRN) of patient
    string id;
    // patient gender identifier
    patient_sex sex;
    // patient birthdate - This is supposedly required, but enough data is missing it that
    // we consider it optional.
    optional<date> birth_date;
    // patient ethnicity descriptor
    string ethnic_group;
    // General comments about the patient
    string comments;
};



// Stores study information (DICOM general & patient study modules).
api(struct)
struct dicom_study
{
    // study date / time
    time study_time;
    // study description
    string description;
    // physician's name (referring physician)
    string physician_name;
    // study name / id
    string name;
    // instance uid
    string instance_uid;
    // accession number
    string accession_number;
};

// Stores treatment equipment information (DICOM general equipment module).
api(struct)
struct dicom_equipment
{
    string manufacturer;
    string institution_name;
};

// DICOM SOP Common & Common instance reference modules
api(struct)
struct dicom_sop_common
{
    // SOP class uid
    string class_uid;
    // SOP instance uid
    string instance_uid;
    // character set
    string specific_char_set;
    // creation date / time
    time creation_time;
};

// Stores data common to all supported DICOM file types (CT, SS, DOSE, ION PLAN).
api(struct)
struct dicom_file
{
    // patient data from file
    patient patient_data;
    // study data
    dicom_study study_data;
    // equipment data
    dicom_equipment equipment_data;
    // frame of reference uid
    string frame_of_ref_uid;
    // position reference indicator (anatomical reference point)
    string position_reference_indicator;
    // SOP common data
    dicom_sop_common sop_data;
};

// Patient position flag for bi-peds (i.e. does not support animals).
api(enum)
enum class patient_position_type
{
    // head first-supine
    HFS,
    // head first-prone
    HFP,
    // feet first-supine
    FFS,
    // feet first-prone
    FFP,
    // head first-decubitus right
    HFDR,
    // head first-decubitus left
    HFDL,
    // feet first-decubitus right
    FFDR,
    // feet first-decubitus left
    FFDL
};

// Stores rt series information (DICOM RT Series module).
api(struct)
struct dicom_rt_series
{
    // modality type of dicom data
    dicom_modality modality;
    // instance uid
    string instance_uid;
    // series number
    int number;
    // Creation date / time of dicom item
    time series_time;
    // Description
    string description;
};

// Stores general series info for images (DICOM General Series module).
api(struct)
struct dicom_general_series : dicom_rt_series
{
    // patient position e.g. HFS
    patient_position_type patient_position;
};

// Approval flags for status of objects.
api(enum)
enum class approval_status
{
    // reviewed and accepted
    APPROVED,
    // not yet reviewed
    UNAPPROVED,
    // reviews and failed to meet standards
    REJECTED
};

void static inline ensure_default_initialization(approval_status &as)
{
    as = approval_status::UNAPPROVED;
}

// Stores the complete approval information for an object.
api(struct)
struct rt_approval
{
    // approval status flag
    approval_status approval;
    // date / time of approval
    optional<time> approval_time;
    // name of reviewer / approver
    string approval_name;
};

// Container for returning DICOM meta data (this is a convenience struct to store commonly
// used data together for efficiency of data access purposes, especially when other types
// are using reference object data).
api(struct)
struct dicom_metadata
{
    // creation date of dicom obj
    date creation_date;
    // creation time of dicom obj
    time creation_time;
    // patient information from dicom obj
    patient patient_data;
    // modality type of dicom obj
    dicom_modality modality;
    // series uid of dicom obj
    string series_uid;
};

}

#endif
