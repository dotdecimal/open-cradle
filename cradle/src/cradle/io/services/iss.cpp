#include <cradle/io/services/iss.hpp>

#include <cradle/io/services/core_services.hpp>
#include <cradle/io/web_io.hpp>

namespace cradle {

string url_type_string(api_type_info const& schema)
{
    switch (schema.type)
    {
     case api_type_info_type::NIL_TYPE:
        return "/nil";
     case api_type_info_type::BOOLEAN_TYPE:
        return "/boolean";
     case api_type_info_type::INTEGER_TYPE:
        return "/integer";
     case api_type_info_type::FLOAT_TYPE:
        return "/float";
     case api_type_info_type::STRING_TYPE:
        return "/string";
     case api_type_info_type::DATETIME_TYPE:
        return "/datetime";
     case api_type_info_type::BLOB_TYPE:
        return "/blob";
     case api_type_info_type::DYNAMIC_TYPE:
        return "/dynamic";
     case api_type_info_type::MAP_TYPE:
      {
        auto const& map_type = as_map_type(schema);
        return "/map" + url_type_string(map_type.key_schema) +
            url_type_string(map_type.value_schema);
      }
     case api_type_info_type::ARRAY_TYPE:
      {
        auto const& array_type = as_array_type(schema);
        return "/array" + url_type_string(array_type.element_schema);
      }
     case api_type_info_type::OPTIONAL_TYPE:
      {
        auto const& optional_type = as_optional_type(schema);
        return "/optional" + url_type_string(optional_type);
      }
     case api_type_info_type::REFERENCE_TYPE:
      {
        auto const& reference_type = as_reference_type(schema);
        return "/reference" + url_type_string(reference_type);
      }
     case api_type_info_type::NAMED_TYPE:
      {
        auto const& named_type = as_named_type(schema);
        return "/named/" + to_string(THINKNODE_ACCOUNT) + "/" + to_string(named_type.app) + "/" + named_type.name;
      }
     case api_type_info_type::STRUCTURE_TYPE:
     case api_type_info_type::UNION_TYPE:
     case api_type_info_type::ENUM_TYPE:
     case api_type_info_type::FUNCTION_TYPE:
     default:
        throw exception("unsupported ISS type");
    }
};

web_request
make_iss_post_request(
    string const& api_url,
    string const& qualified_type,
    blob const& data,
    framework_context const& fc)
{
    return make_post_request(api_url + "/iss/" + qualified_type +
        "?context=" + fc.context_id, data,
        make_header_list("Content-Type: application/octet-stream"));
}

string
post_iss_data(
    check_in_interface& check_in,
    progress_reporter_interface& reporter,
    web_connection &connection,
    web_session_data session,
    framework_context context,
    blob const& data,
    string const& qualified_type)
{
    web_response iss_response =
        perform_web_request(
            check_in,
            reporter,
            connection,
            session,
            make_iss_post_request(context.framework.api_url, qualified_type, data, context));

    return
        from_value<cradle::iss_response>(parse_json_response(iss_response)).id;
}

}
