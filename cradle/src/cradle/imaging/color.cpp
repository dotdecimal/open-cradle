#include <cradle/imaging/color.hpp>

namespace alia {

cradle::raw_type_info get_type_info(rgb8 const& x)
{
    return cradle::raw_type_info(cradle::raw_kind::NAMED_TYPE_REFERENCE,
        cradle::any(
            cradle::raw_named_type_reference(THINKNODE_TYPES_APP, string("rgb8"))));
}
cradle::raw_type_info get_proper_type_info(rgb8 const& x)
{
    using cradle::get_type_info;
    std::vector<cradle::raw_structure_field_info> fields;
    fields.push_back(cradle::raw_structure_field_info(
        "r", "Red", get_type_info(uint8_t())));
    fields.push_back(cradle::raw_structure_field_info(
        "g", "Green", get_type_info(uint8_t())));
    fields.push_back(cradle::raw_structure_field_info(
        "b", "Blue", get_type_info(uint8_t())));
    return cradle::raw_type_info(cradle::raw_kind::STRUCTURE,
        cradle::any(cradle::raw_structure_info("rgb8", "RGB triplet", fields)));
}

cradle::raw_type_info get_type_info(rgba8 const& x)
{
    return cradle::raw_type_info(cradle::raw_kind::NAMED_TYPE_REFERENCE,
        cradle::any(
            cradle::raw_named_type_reference(THINKNODE_TYPES_APP, string("rgba8"))));
}
cradle::raw_type_info get_proper_type_info(rgba8 const& x)
{
    using cradle::get_type_info;
    std::vector<cradle::raw_structure_field_info> fields;
    fields.push_back(cradle::raw_structure_field_info(
        "r", "Red", get_type_info(uint8_t())));
    fields.push_back(cradle::raw_structure_field_info(
        "g", "Green", get_type_info(uint8_t())));
    fields.push_back(cradle::raw_structure_field_info(
        "b", "Blue", get_type_info(uint8_t())));
    fields.push_back(cradle::raw_structure_field_info(
        "a", "Alpha", get_type_info(uint8_t())));
    return cradle::raw_type_info(cradle::raw_kind::STRUCTURE,
        cradle::any(cradle::raw_structure_info("rgba8", "RGB triplet", fields)));
}

}

namespace cradle {

rgb8
choose_new_color(
    std::vector<rgb8> const& palette,
    std::vector<rgb8> const& already_in_use)
{
    // This just tries to maximize the minimum RGB distance from the other
    // colors.
    // TODO: Handle the case where the palette is entirely used up.
    rgb8 best_color;
    double best_distance = -1;
    for (auto const& p : palette)
    {
        auto p_distance = std::numeric_limits<double>::max();
        for (auto const& a : already_in_use)
        {
            auto distance =
                length2(make_vector<double>(p.r, p.g, p.b) -
                    make_vector<double>(a.r, a.g, a.b));
            if (distance < p_distance)
                p_distance = distance;
        }
        if (p_distance > best_distance)
        {
            best_color = p;
            best_distance = p_distance;
        }
    }
    return best_color;
}

}
