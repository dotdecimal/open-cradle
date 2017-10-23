#ifndef CRADLE_RT_PLAN_HPP
#define CRADLE_RT_PLAN_HPP

#include <cradle/rt/common.hpp>

#include <cradle/imaging/slicing.hpp>
#include <cradle/imaging/variant.hpp>
#include <cradle/geometry/polygonal.hpp>

#include <cradle/api.hpp>

namespace cradle {

void static inline ensure_default_initialization(patient_position_type &pp)
{
    pp = patient_position_type::HFS;
}

// This defines the placement of a spot.
api(struct)
struct spot_placement
{
    // The machine energy setting for the spot.
    double energy;
    // The BEV position of the spot (at isocenter).
    vector2d position;
};

typedef std::vector<spot_placement> spot_placement_list;

// This defines both the placement and the weight of a spot.
api(struct)
struct weighted_spot : spot_placement
{
    // The fluence (weight) of this spot.
    double fluence;
};

typedef std::vector<weighted_spot> weighted_spot_list;

// information stored in a dicom control point sequence
api(struct)
struct pbs_spot_layer
{
    // Number of scan spot positions
    unsigned num_spot_positions;
    // List of PBS spot placements
    weighted_spot_list spots;
    // Size of spot
    vector2d spot_size;
    // Number of times layer is painted
    unsigned num_paintings;
    // Spot tune id
    unsigned spot_tune_id;
};

// Stores information from a dicom control point sequence.
api(struct)
struct rt_control_point
{
    // control point number, index (zero based)
    unsigned number;
    // meterset weight
    double meterset_weight;
    // meterset rate
    double meterset_rate;
    // nominal beam energy
    double nominal_beam_energy;
    // nominal beam energy unit
    string nominal_beam_energy_unit;
    // gantry angle
    double gantry_angle;
    // gantry rotation direction
    string gantry_rotation_direction;
    // gantry pitch angle
    double gantry_pitch_angle;
    // gantry pitch direction
    string gantry_pitch_direction;
    // beam limiting device angle
    double beam_limiting_device_angle;
    // beam limiting direction
    string beam_limiting_direction;
    // patient support angle
    double patient_support_angle;
    // patient support direction
    string patient_support_direction;
    // source to surface distance
    double source_to_surface_distance;
    // Table top pitch angle
    double table_top_pitch_angle;
    // table top pitch direction
    string table_top_pitch_direction;
    // table top roll angle
    double table_top_roll_angle;
    // table top roll direction
    string table_top_roll_direction;
    // snout position
    double snout_position;
    // isocenter position
    vector<3, double> isocenter_position;
    // surface entry point
    vector<3, double> surface_entry_point;
    // Spot scan tune id
    string spot_scan_tune;
    // Layer of PBS spots
    pbs_spot_layer layer;
};

void static inline ensure_default_initialization(rt_control_point &v)
{
    v.surface_entry_point = make_vector(-1.e20, -1.e20, -1.e20);
    v.nominal_beam_energy = -1.;
    v.nominal_beam_energy_unit = "";
    v.meterset_weight = -1.;
    v.meterset_rate = -1.;
}

// Mounting positions of beam devices
api(enum)
enum class rt_mounting_position
{
    // the block is mounted on the side of the Block Tray which is towards the patient.
    PATIENT_SIDE,
    // the block is mounted on the side of the Block Tray which is towards the radiation source.
    SOURCE_SIDE,
    // only for rangeCompensators
    DOUBLE_SIDED
};

void static inline ensure_default_initialization(rt_mounting_position &v)
{
    v = rt_mounting_position::SOURCE_SIDE;
}

// Holds information stored in a dicom range compensator sequence.
api(struct)
struct rt_ion_rangecompensator
{
    // range compensator name / id
    string name;
    // device number
    unsigned number;
    // range compensator material
    string material;
    // range compensator divergence (true not supported)
    bool divergent;
    // range compensator beam mounting position
    rt_mounting_position mounting_position;
    // downstream edge location of range compensator
    double downstream_edge;
    // offset between point position from first column to second column. This value gives the "honey-comb" appearance.
    double column_offset;
    // relative stopping power ratio
    double relative_stopping_power;
    // upper left corner position of the range compensator
    vector2d position;
    // spacing between each data pixel
    vector2d pixel_spacing;
    // image of pixel values of range compensator
    image<2,double,shared> data;
};

// Aperture blocking type
api(enum)
enum class rt_ion_block_type
{
    // blocking material is outside contour
    APERTURE,
    // blocking material is inside contour
    SHIELDING
};

void static inline ensure_default_initialization(rt_ion_block_type &v)
{
    v = rt_ion_block_type::APERTURE;
}

// Holds information stored in a dicom ion block sequence.
api(struct)
struct rt_ion_block
{
    // aperture name
    string name;
    // aperture description, used as accessory code in DICOM
    string description;
    // aperture material
    string material;
    // aperture number
    unsigned number;
    // aperture divergence
    bool divergent;
    // downstream edge location of aperture
    double downstream_edge;
    // physical aperture thickness
    double thickness;
    // aperture beam mounting position
    rt_mounting_position position;
    // aperture type (e.g. aperture or shielding)
    rt_ion_block_type block_type;
    // contour data
    polyset data;
};

// Holds information stored in a dicom snout sequence.
api(struct)
struct rt_snout
{
    // snout id
    string id;
    // snout accessory code
    string accessory_code;
};

// Dicom radiation type enum.
api(enum)
enum class rt_radiation_type
{
    // proton beam
    PROTON,
    // photon beam
    PHOTON,
    // electron beam
    ELECTRON,
    // neutron beam
    NEUTRON
};

void static inline ensure_default_initialization(rt_radiation_type &v)
{
    v = rt_radiation_type::PROTON;
}

// Motion characteristic of the beam.
api(enum)
enum class rt_ion_beam_type
{
    // all beam parameters remain unchanged during delivery
    STATIC,
    // one or more beam parameters changes during delivery
    DYNAMIC
};

// Method of beam scanning to be used during treatment.
api(enum)
enum class rt_ion_beam_scan_mode
{
    // no beam scanning is performed (SOBP)
    NONE,
    // the beam is scanned between control points to create
    // a uniform lateral fluence distribution across the field
    UNIFORM,
    // the beam is scanned between control points to create
    // a modulated lateral fluence distribution across the field (PBS)
    MODULATED
};

void static inline ensure_default_initialization(rt_ion_beam_scan_mode &v)
{
    v = rt_ion_beam_scan_mode::NONE;
}

// Range shifter types enum.
api(enum)
enum class rt_range_shifter_type
{
    // device is variable thickness and is composed of opposing
    // sliding wedges, water column or similar mechanism
    ANALOG,
    // device is composed of different thickness materials that can
    // be moved in or out of the beam in various stepped
    // combinations
    BINARY,
};

void static inline ensure_default_initialization(rt_range_shifter_type &v)
{
    v = rt_range_shifter_type::BINARY;
}

// Holds range shifter information.
api(struct)
struct rt_ion_range_shifter
{
    // range shifter number
    unsigned number;
    // range shifter id
    string id;
    // range shifter type
    rt_range_shifter_type type;
    // range shifter accessory ccode
    string accessory_code;
};

// Holds information stored in a dicom ion beam sequence.
api(struct)
struct rt_ion_beam
{
    // beam number
    unsigned beam_number;
    // beam name
    string name;
    // beam description
    string description;
    // treatment machine name
    string treatment_machine;
    // treatment machine manufacturer
    string machine_manufacturer_name;
    // treatment machine model
    string machine_model_name;
    // primary dosimeter unit
    string primary_dosimeter_unit;
    // treatment delivery type
    string treatment_delivery_type;
    // static or Dynamic beam type
    rt_ion_beam_type beam_type;
    // scanning mode of the beam
    rt_ion_beam_scan_mode beam_scan_mode;
    // radiation beam type
    rt_radiation_type radiation_type;
    // referenced_patient_setup id
    unsigned referenced_patient_setup;
    // referenced_tolerance_table id
    unsigned referenced_tolerance_table;
    // virtual sad
    vector2d virtual_sad;
    // patient support type (TABLE or CHAIR)
    string patient_support_type;
    // final meterset weight
    double final_meterset_weight;
    // treatment snout
    rt_snout snout;
    // aperture block
    optional<rt_ion_block> block;
    // list of range shifters
    std::vector<rt_ion_range_shifter> shifters;
    // list of range compensators
    std::vector<rt_ion_rangecompensator> compensators;
    // list of control points
    std::vector<rt_control_point> control_points;
};

// Holds information stored in a dicom dose reference sequence.
api(struct)
struct rt_dose_reference
{
    // dose number
    unsigned number;
    // dose ref id (unique to patient so it can be used across plans)
    string uid;
    // structure type
    string structure_type;
    // description
    string description;
    // type (only TARGET supported)
    string dose_type;
    // referenced ROI number
    unsigned ref_roi_number;
    // delivery max dose
    double delivery_max_dose;
    // target prescription dose
    double target_rx_dose;
    // target minimum dose
    double target_min_dose;
    // target maximum dose
    double target_max_dose;
    // target underdose volume fraction
    double target_underdose_vol_fraction;
    // point coordinates
    vector3d point_coordinates;
};

// Holds information stored in a dicom tolerance table sequence.
api(struct)
struct rt_tolerance_table
{
    // number (unique within the plan)
    unsigned number;
    // label
    string label;
    // gantry angle tolerance
    double gantry_angle_tol;
    // beam limiting device angle tolerance
    double beam_limiting_angle_tol;
    // patient support angle tolerance
    double patient_support_angle_tol;
    // table top vertical position tolerance
    double table_top_vert_position_tol;
    // table top longitudinal position tolerance
    double table_top_long_position_tol;
    // table top latitude position tolerance
    double table_top_lat_position_tol;
    // table top pitch angle tolerance
    double table_top_pitch_tol;
    // table top roll angle tolerance
    double table_top_roll_tol;
    // table top snout position tolerance
    double snout_position_tol;
    // list of beam device tolerances (sequence)
    std::map<string, double> limiting_device_tol_list;
};

// Holds information stored in a dicom patient setup sequence.
api(struct)
struct rt_patient_setup
{
    // setup number
    unsigned setup_number;
    // patient position (e.g. hfs)
    patient_position_type position;
    // setup description/label
    string setup_label;
    // table top vertical setup displacement
    double table_top_vert_setup_dis;
    // table top longitudinal setup displacement
    double table_top_long_setup_dis;
    // table top lateral setup displacement
    double table_top_lateral_setup_dis;
    // setup technique
    string setup_technique;
};

void static inline ensure_default_initialization(rt_patient_setup &v)
{
    v.table_top_vert_setup_dis = 0.;
    v.table_top_long_setup_dis = 0.;
    v.table_top_lateral_setup_dis = 0.;
}

// Stores information stored in a dicom reference beam sequence.
api(struct)
struct rt_ref_beam
{
    // beam number
    unsigned beam_number;
    // beam dose specification point
    vector3d dose_specification_point;
    // beam dose
    double beam_dose;
    // beam meterset
    double beam_meterset;
};

void static inline ensure_default_initialization(rt_ref_beam &v)
{
    v.dose_specification_point = make_vector(-1.e20, -1.e20, -1.e20);
    v.beam_meterset = -1.;
}

// Stores information stored in a dicom fraction group sequence.
api(struct)
struct rt_fraction
{
    // fraction number
    unsigned number;
    // number of planned fractions
    unsigned number_planned_fractions;
    // list of beam references
    std::vector<rt_ref_beam> ref_beams;
    // fraction group description
    string description;
    // number of treatment sessions per day (typically 1)
    unsigned fractions_per_day;
    // length of fraction pattern units (weeks)
    unsigned fraction_pattern_length;
    // binary string describing the fraction daily delivery pattern (length of this string
    // should be (fractions_per_day x fraction_pattern_length x 7)
    string fraction_pattern;
};

// Stores radiotherapy ion plan data (DICOM-like).
api(struct)
struct rt_ion_plan : dicom_file
{
    // RT Series data
    dicom_rt_series series;
    // plan label
    string label;
    // plan name
    string name;
    // plan description
    string description;
    // Instance number.
    int instance_number;
    // plan date/time
    time plan_time;
    // plan geometry (PATIENT or TREATMENT_DEVICE)
    string geometry;
    // referenced RT SS sequence
    ref_dicom_item ref_ss_data;
    // list of prescriptions (dose references)
    std::vector<rt_dose_reference> prescriptions;
    // list of patient setups
    std::vector<rt_patient_setup> patient_setups;
    // list of fractions
    std::vector<rt_fraction> fractions;
    // list of tolerance tables
    std::vector<rt_tolerance_table> tolerance_tables;
    // list of beams
    std::vector<rt_ion_beam> beams;
    // approval status information
    rt_approval approval_info;
};


}

#endif