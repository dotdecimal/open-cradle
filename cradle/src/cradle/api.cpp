#include <cradle/api.hpp>
#include <cradle/api_index.hpp>
#include <boost/uuid/sha1.hpp>
#include <cradle/io/generic_io.hpp>
#include <cradle/encoding.hpp>

#include <boost/algorithm/string/split.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace cradle {

string
generate_function_uid(
    string const& name,
    std::vector<api_function_parameter_info> const& parameters,
    unsigned revision)
{
    api_function_uid_contents uid(name, parameters, revision);
    auto json = value_to_json(to_value(uid));

    boost::uuids::detail::sha1 sha1;
    sha1.process_bytes(json.c_str(), json.size());
    unsigned int digest[5];
    sha1.get_digest(digest);

    return base64_encode(reinterpret_cast<uint8_t*>(digest), 20,
        get_mime_base64_character_set());
}

api_structure_info static
make_api_structure_info(raw_structure_info const& raw)
{
    api_structure_info info;
    for (auto const& field : raw.fields)
    {
        info.fields[field.name] =
            field.type.kind == raw_kind::OMISSIBLE ?
                api_structure_field_info(
                    field.description,
                    make_api_type_info(
                        unsafe_any_cast<raw_type_info>(field.type.info)),
                    some(true)) :
                api_structure_field_info(
                    field.description,
                    make_api_type_info(field.type),
                    none);
    }
    return info;
}

api_union_info static
make_api_union_info(raw_union_info const& raw)
{
    api_union_info info;
    for (auto const& member : raw.members)
    {
        info.members[member.name] =
            api_union_member_info(
                member.description,
                make_api_type_info(member.type));
    }
    return info;
}

api_enum_info static
make_api_enum_info(raw_enum_info const& raw)
{
    api_enum_info info;
    for (auto const& value : raw.values)
    {
        info.values[value.name] =
            api_enum_value_info(
                value.description);
    }
    return info;
}

api_type_info static
make_api_simple_type(raw_simple_type raw)
{
    switch (raw)
    {
     case raw_simple_type::BLOB:
        return make_api_type_info_with_blob_type(api_blob_type());
     case raw_simple_type::BOOLEAN:
        return make_api_type_info_with_boolean_type(api_boolean_type());
     case raw_simple_type::DYNAMIC:
        return make_api_type_info_with_dynamic_type(api_dynamic_type());
     case raw_simple_type::INTEGER:
        return make_api_type_info_with_integer_type(api_integer_type());
     case raw_simple_type::FLOAT:
        return make_api_type_info_with_float_type(api_float_type());
     case raw_simple_type::STRING:
        return make_api_type_info_with_string_type(api_string_type());
     case raw_simple_type::DATETIME:
        return make_api_type_info_with_datetime_type(api_datetime_type());
     case raw_simple_type::NIL:
     default:
        return make_api_type_info_with_nil_type(api_nil_type());
    }
}

api_named_type_reference
make_api_named_type_reference(raw_named_type_reference const& raw)
{
    return api_named_type_reference(raw.app, raw.type);
}

api_type_info make_api_type_info(raw_type_info const& raw)
{
    switch (raw.kind)
    {
     case raw_kind::ARRAY:
      {
        auto const& raw_info = unsafe_any_cast<raw_array_info>(raw.info);
        return make_api_type_info_with_array_type(
            api_array_info(raw_info.size,
                make_api_type_info(raw_info.element_type)));
      }
     case raw_kind::ENUM:
      {
        auto const& raw_info = unsafe_any_cast<raw_enum_info>(raw.info);
        return make_api_type_info_with_enum_type(
            make_api_enum_info(raw_info));
      }
     case raw_kind::MAP:
      {
        auto const& raw_info = unsafe_any_cast<raw_map_info>(raw.info);
        return make_api_type_info_with_map_type(
            api_map_info(
                make_api_type_info(raw_info.key),
                make_api_type_info(raw_info.value)));
      }
     case raw_kind::OPTIONAL:
      {
        auto const& raw_info = unsafe_any_cast<raw_type_info>(raw.info);
        return make_api_type_info_with_optional_type(
            make_api_type_info(raw_info));
      }
     case raw_kind::STRUCTURE:
      {
        auto const& raw_info = unsafe_any_cast<raw_structure_info>(raw.info);
        return make_api_type_info_with_structure_type(
            make_api_structure_info(raw_info));
      }
     case raw_kind::UNION:
      {
        auto const& raw_info = unsafe_any_cast<raw_union_info>(raw.info);
        return make_api_type_info_with_union_type(
            make_api_union_info(raw_info));
      }
     case raw_kind::DATA_REFERENCE:
      {
        auto const& raw_info = unsafe_any_cast<raw_type_info>(raw.info);
        return make_api_type_info_with_reference_type(
            make_api_type_info(raw_info));
      }
     case raw_kind::NAMED_TYPE_REFERENCE:
      {
        auto const& raw_info =
            unsafe_any_cast<raw_named_type_reference>(raw.info);
        return make_api_type_info_with_named_type(
            make_api_named_type_reference(raw_info));
      }
     case raw_kind::SIMPLE:
      {
        auto const& raw_info = unsafe_any_cast<raw_simple_type>(raw.info);
        return make_api_simple_type(raw_info);
      }
     case raw_kind::OMISSIBLE:
        throw exception("omissible used outside of structure field");
     default:
        throw exception("invalid raw_kind");
    }
}

void register_api_dependency_type(api_implementation& api,
    string const& account, string const& app, string const& version)
{
        api.dependencies.push_back(api_dependency_type_info(account, app, version));
}

void register_api_provider_type(api_implementation& api,
    string const& tag)
{
        api_provider_type_info p;
        p.f_private.image.tag = tag;
        api.provider = p;
}

void register_api_previous_release_version(api_implementation& api,
    string const& version)
{
    api_previous_release_info previous_release;
    previous_release.version = version;
    api.previous_release_version = previous_release;
}

void register_api_mutation_type(api_implementation& api,
    string const& description,
    string const& upgrade_version, string const& upgrade_type,
    string const& body)
{
    api_upgrade_type_info up;
    up.name = upgrade_type + string("_") + upgrade_version;
    up.description = description;

    up.schema = make_upgrade_type_info_with_mutation_type(parse_json_value(body));
    api.upgrades.push_back(up);
}

void register_api_named_type(api_implementation& api,
    string const& name, unsigned version, string const& description,
    api_type_info const& info, upgrade_type upgrade)
{
    api.types.push_back(api_named_type_implementation_info(name, description, upgrade, info));
}

void register_api_record_type(api_implementation& api,
    string const& record_name, string const& description,
    string const& account, string const& app, string const& name)
{
    api_named_record_type_info rti;
    rti.description = description;
    rti.name = record_name;

    api_record_named_type_info rni;
    if (account.size() > 1) { rni.account = account; }
    if (app.size() > 1) { rni.app = app; }
    rni.name = name;

    api_record_info ri;
    ri.schema.named_type = rni;

    rti.schema = make_api_type_info_with_record_type(ri);

    api.records.push_back(rti);
}

api_upgrade_type_info make_upgrade_function_api_info(api_function_ptr const& f)
{
    if (!is_function_type(f->api_info.schema))
    {
        throw exception("Upgrade api info found that is not a function: " +
            f->api_info.name);
    }
    auto fs = as_function_type(f->api_info.schema);
    if (!is_named_type(fs.returns.schema))
    {
        throw exception("Upgrade api info found that doesn't return a named type: " +
            f->api_info.name);
    }
    auto return_type = as_named_type(fs.returns.schema);
    api_upgrade_type_info up;
    up.name = "upgrade_" + to_string(return_type.name) + string("_") +
        f->implementation_info.upgrade_version;
    up.description = "Upgrade: " + f->api_info.description;
    api_upgrade_function_info upfi;
    upfi.function = f->api_info.name;
    upfi.type = return_type.name;
    upfi.version = f->implementation_info.upgrade_version;
    up.schema = make_upgrade_type_info_with_upgrade_type(upfi);

    return up;
}

void register_api_function(api_implementation& api,
    api_function_ptr const& f)
{
    string const& uid = f->implementation_info.uid;
    auto existing = api.functions.find(uid);
    if (existing != api.functions.end())
    {
        throw exception("duplicate function UID detected:\n" +
            uid + "\n" +
            existing->second->api_info.name + "\n" +
            f->api_info.name);
    }
    api.functions[uid] = f;

    if (is_upgrade(f->implementation_info))
    {
        api.upgrades.push_back(make_upgrade_function_api_info(f));
    }
}

bool
function_is_upgrade(
    std::vector<api_upgrade_type_info> const& upgrades,
    string function_name)
{
        bool is_upgrade = false;
        for (auto const& upgrade : upgrades)
        {
                if (upgrade.schema.type == upgrade_type_info_type::UPGRADE_TYPE)
                {
                        auto ug = as_upgrade_type(upgrade.schema);
                        if (ug.function == function_name)
                        {
                                is_upgrade = true;
                                break;
                        }
                }
        }
        return is_upgrade;
}

api_named_type_info
remove_upgrade_info_from_named_type(api_named_type_implementation_info const& ut)
{
    return api_named_type_info(ut.name, ut.description, ut.schema);
}

std::vector<api_named_type_info>
get_api_named_type_documentation_definition(api_implementation const& api)
{
    std::vector<api_named_type_info> api_types;
    for_each(api.types.begin(), api.types.end(),
        [&](api_named_type_implementation_info const& ut)
        {
            api_types.push_back(remove_upgrade_info_from_named_type(ut));
        });
    return api_types;
}

api_documentation
get_api_documentation(
    api_implementation const& api,
    bool include_upgrade_functions)
{
    std::vector<api_function_info> function_info;
    for (auto const& f : api.functions)
    {
        if (!include_upgrade_functions ||
            function_is_upgrade(api.upgrades, f.second->api_info.name))
        {
                auto info = f.second->api_info;
                // hard code all api functions to x32 for testing
                //info.execution_class = "cpu.x32";
                function_info.push_back(info);
        }
    }
    return api_documentation(
            get_api_named_type_documentation_definition(api),
            function_info,
            generate_api_upgrades(api),
            api.dependencies,
            api.provider,
            api.records);
}

string get_api_implementation_documentation(api_implementation const& api)
{
    std::vector<api_function_info> function_info;
    for (auto const& f : api.functions)
    {
        if (function_is_upgrade(api.upgrades, f.second->api_info.name))
        {
                function_info.push_back(f.second->api_info);
        }
    }

    string json = value_to_json(to_value(api.provider));
    boost::replace_all(json, "f_private", "private");
    return json;
}

string get_manifest_json(api_implementation const& api)
{
    string json = value_to_json(to_value(get_api_documentation(api, false)));
    // "private" is a C++ keyword so we can only use it once the manifest is in
    // its JSON form.
    boost::replace_all(json, "f_private", "private");
    return json;
}

string get_manifest_json_with_upgrades(api_implementation const& api)
{
    string json = value_to_json(to_value(get_api_documentation(api, true)));
    // "private" is a C++ keyword so we can only use it once the manifest is in
    // its JSON form.
    boost::replace_all(json, "f_private", "private");
    return json;
}

api_documentation get_api_upgrade_documentation(api_implementation const& api)
{
    std::vector<api_function_info> function_info;
    for (auto const& f : api.functions)
    {
        if (function_is_upgrade(api.upgrades, f.second->api_info.name))
        {
                function_info.push_back(f.second->api_info);
        }
    }
    return api_documentation(
            get_api_named_type_documentation_definition(api),
            function_info,
            generate_api_upgrades(api),
            api.dependencies,
            api.provider,
            api.records);
}

std::vector<api_upgrade_type_info>
generate_api_upgrades(api_implementation const& api)
{
    std::vector<api_upgrade_type_info> upgrades;

    for(auto const& st : api.types)
    {
        if (st.upgrade == upgrade_type::FUNCTION)
        {
            api_upgrade_type_info up;
            up.name = string("upgrade_value_") + st.name;
            up.description = string("upgrade for type ") + st.name;
            api_upgrade_function_info up_fun;
            up_fun.function = string("upgrade_value_") + st.name;
            up_fun.type = st.name;
            up_fun.version = api.previous_release_version.version;
            up.schema = make_upgrade_type_info_with_upgrade_type(up_fun);
            upgrades.push_back(up);
        }
    }

    return upgrades;
}

api_function_interface const&
find_function_by_name(api_implementation const& api, string const& name)
{
    for (auto const& f : api.functions)
    {
        if (f.second->api_info.name == name)
            return *f.second;
    }
    throw undefined_function(name);
}

api_function_interface const&
find_function_by_uid(api_implementation const& api, string const& uid)
{
    auto f = api.functions.find(uid);
    if (f == api.functions.end())
        throw undefined_function(uid);
    return *f->second;
}

api_implementation
merge_apis(api_implementation const& a, api_implementation const& b)
{
    api_implementation merged;
    for (auto const& i : a.types)
        merged.types.push_back(i);
    for (auto const& i : b.types)
        merged.types.push_back(i);
    for (auto const& i : a.functions)
        register_api_function(merged, i.second);
    for (auto const& i : b.functions)
        register_api_function(merged, i.second);
    for (auto const& i : a.upgrades)
        merged.upgrades.push_back(i);
    for (auto const& i : b.upgrades)
        merged.upgrades.push_back(i);

    for (auto const& i : a.dependencies)
        merged.dependencies.push_back(i);
    for (auto const& i : b.dependencies)
        merged.dependencies.push_back(i);

    for (auto const& i : a.records)
        merged.records.push_back(i);
    for (auto const& i : b.records)
        merged.records.push_back(i);

    if (a.provider)
        merged.provider = a.provider;
    else if (b.provider)
        merged.provider = b.provider;

    merged.previous_release_version = b.previous_release_version;

    return merged;
}

// THE CRADLE API

api_implementation get_cradle_api()
{
    api_implementation api;
    CRADLE_REGISTER_APIS(api);
    return api;
}

api_documentation get_api_documentation()
{
    return get_api_documentation(get_cradle_api());
}

}
