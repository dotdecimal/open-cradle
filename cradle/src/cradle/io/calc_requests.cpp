#include <cradle/io/calc_requests.hpp>

#include <algorithm>
#include <unordered_map>

#include <boost/range/adaptor/reversed.hpp>

namespace cradle {

calculation_request static
generate_thinknode_request_with_substitutions(
    std::unordered_map<untyped_request,calculation_request> const& substitutions,
    untyped_request const& request,
    bool is_embedded = false)
{
    // Check if this request has been substituted for.
    {
        auto substitution = substitutions.find(request);
        if (substitution != substitutions.end())
            return substitution->second;
    }

    // This defines the recursive call that we'll make if necessary.
    auto recurse =
        [&](untyped_request const& request)
        {
            return
                generate_thinknode_request_with_substitutions(
                    substitutions, request, true);
        };

    // Generate the actual Thinknode request.
    switch (request.type)
    {
     case request_type::IMMEDIATE:
        return make_calculation_request_with_value(
            as_immediate(request).ptr->as_value());
     case request_type::FUNCTION:
      {
        auto const& spec = as_function(request);
        // If this function is supposed to be reported, it can't be embedded
        // in another request because then it won't have a variable assigned
        // and we won't know its ID automatically from submitting it.
        if (is_reported(*spec.function) && is_embedded)
        {
            throw exception("internal error: 'reported' function embedded improperly");
        }
        // Construct with an empty argument list and then swap in the args to
        // avoid copying them.
        function_application fa(
            spec.function->implementation_info.account_id,
            spec.function->implementation_info.app_id,
            spec.function->api_info.name,
            std::vector<calculation_request>(),
            spec.function->implementation_info.level);
        auto mapped_args = map(recurse, spec.args);
        swap(fa.args, mapped_args);
        return make_calculation_request_with_function(std::move(fa));
      }
     case request_type::ARRAY:
      {
        auto result_type = request.result_interface->type_info();
        assert(result_type.kind == raw_kind::ARRAY);
        // Construct with an empty item list and then swap in the args to
        // avoid copying them.
        calculation_array_request array(
            std::vector<calculation_request>(),
            make_api_type_info(
                unsafe_any_cast<raw_array_info>(result_type.info).
                    element_type));
        auto mapped_items = map(recurse, as_array(request));
        swap(array.items, mapped_items);
        return make_calculation_request_with_array(std::move(array));
      }
     case request_type::STRUCTURE:
      {
        // Construct with an empty field list and then swap in the fields to
        // avoid copying them.
        calculation_object_request object(
            std::map<string,calculation_request>(),
            make_api_type_info(request.result_interface->type_info()));
        auto mapped_fields = map(recurse, as_structure(request).fields);
        swap(object.properties, mapped_fields);
        return make_calculation_request_with_object(std::move(object));
      }
     case request_type::PROPERTY:
        return make_calculation_request_with_property(
            calculation_property_request(
                recurse(as_property(request).record),
                make_api_type_info(request.result_interface->type_info()),
                make_calculation_request_with_value(
                    to_value(as_property(request).field))));
     case request_type::UNION:
      {
        auto result_type = request.result_interface->type_info();
        calculation_object_request object;
        object.properties[as_union(request).member_name] =
            recurse(as_union(request).member_request);
        object.schema =
            make_api_type_info(request.result_interface->type_info());
        return make_calculation_request_with_object(std::move(object));
      }
     case request_type::SOME:
      {
        calculation_object_request object;
        object.properties["some"] = recurse(as_some(request).value);
        object.schema =
            make_api_type_info(request.result_interface->type_info());
        return make_calculation_request_with_object(std::move(object));
      }
     case request_type::REQUIRED:
        return make_calculation_request_with_property(
            calculation_property_request(
                recurse(as_required(request).optional_value),
                make_api_type_info(request.result_interface->type_info()),
                make_calculation_request_with_value(
                    to_value(string("some")))));
     case request_type::ISOLATED:
        // This doesn't automatically count as an embedded call because it
        // directly returns the result of the recursive call.
        return
            generate_thinknode_request_with_substitutions(
                substitutions,
                as_isolated(request),
                is_embedded);
     case request_type::REMOTE_CALCULATION:
        // This doesn't automatically count as an embedded call because it
        // directly returns the result of the recursive call.
        return
            generate_thinknode_request_with_substitutions(
                substitutions,
                as_remote_calc(request),
                is_embedded);
     case request_type::META:
     {
        meta_calculation_request meta;
        meta.generator = recurse(as_meta(request));
        meta.schema =
            make_api_type_info(request.result_interface->type_info());
        return make_calculation_request_with_meta(std::move(meta));
     }
     case request_type::IMMUTABLE:
        return make_calculation_request_with_reference(as_immutable(request));
     case request_type::OBJECT:
        return make_calculation_request_with_reference(as_object(request));
     default:
        assert(0);
        throw cradle::exception("internal error: request type has no Thinknode equivalent");
    }
}

calculation_request
as_thinknode_request(untyped_request const& request)
{
    return
        generate_thinknode_request_with_substitutions(
            std::unordered_map<untyped_request,calculation_request>(),
            request);
}

// Is the given request one that should have its progress reported to the
// user? - Note that this only applies to the top level of the request.
bool static
is_reported_request(untyped_request const& request)
{
    switch (request.type)
    {
     case request_type::FUNCTION:
        return is_reported(*as_function(request).function);
     case request_type::ISOLATED:
        return is_reported_request(as_isolated(request));
     case request_type::REMOTE_CALCULATION:
        return is_reported_request(as_remote_calc(request));
     default:
        return false;
    }
}

// This records the fact that we've assigned a specific value to a variable.
struct variable_declaration
{
    string name;
    calculation_request value;
};

calculation_request
generate_compact_thinknode_request(
    composition_cache_entry_map const& cache_entries,
    std::vector<id_interface const*> const& order_added,
    untyped_request const& request,
    std::vector<string>* reported_variables)
{
    std::unordered_map<untyped_request,calculation_request> substitutions;
    substitutions.reserve(cache_entries.size());
    std::vector<variable_declaration> declarations;
    // +1 here is because we may need to add the core request (see below).
    declarations.reserve(cache_entries.size() + 1);

    // Each entry was intended to store a result that's reused, so assign variables to
    // each of them.
    //
    // Note that we apply the existing substitutions as we build the requests for new
    // variables. The order we use guarantees that higher-level requests will only be
    // processed after substitutions have already been created for their subrequests.
    //
    int variable_number = 0;
    for (id_interface const* key : order_added)
    {
        auto entry = cache_entries.find(key);
        if (entry == cache_entries.end())
            throw exception("internal error: missing composition_cache_entry");
        untyped_request const& subrequest = entry->second.result;

        // Apply the existing substitutions and record the declaration for the new
        // variable.
        auto substituted =
            generate_thinknode_request_with_substitutions(substitutions, subrequest);
        // Thinknode doesn't like variables being assigned directly to other variables,
        // and such declarations are useless anyway, so just ignore them.
        if (substituted.type != calculation_request_type::VARIABLE)
        {
            declarations.push_back(variable_declaration());
            auto& declaration = declarations.back();
            declaration.value = std::move(substituted);
            declaration.name = "x" + to_string(variable_number++);

            // If the request is for a 'reported' calculation, record this variable in the
            // list of reported variables.
            if (reported_variables && is_reported_request(subrequest))
            {
                reported_variables->push_back(declaration.name);
            }

            // Also record the substitution.
            substitutions[subrequest] =
                make_calculation_request_with_variable(declaration.name);
        }
    }

    // Now translate the request using those substitutions.
    auto core_request =
        generate_thinknode_request_with_substitutions(substitutions, request);

    // If the core request is supposed to be reported, then add a variable to represent
    // that as well and record the new variable as a reported one.
    if (is_reported_request(request))
    {
        declarations.push_back(variable_declaration());
        auto& declaration = declarations.back();
        std::swap(declaration.value, core_request);
        declaration.name = "x" + to_string(variable_number++);

        core_request = make_calculation_request_with_variable(declaration.name);

        reported_variables->push_back(declaration.name);
    }

    // Finally, wrap the core request in 'let' requests for all the variable declarations,
    // working from the inside out.
    calculation_request wrapped_request = core_request;
    for (auto& decl : boost::adaptors::reverse(declarations))
    {
        std::map<string,calculation_request> variables;
        // We don't need the one in the list anymore, so just swap it into here to avoid
        // the copy.
        swap(variables[decl.name], decl.value);
        // Again, swap things in to avoid copying.
        let_calculation_request let;
        swap(let.variables, variables);
        swap(let.in, wrapped_request);
        // Finally, accumulate the LET into wrapped_request.
        set_to_let(wrapped_request, std::move(let));
    }

    return wrapped_request;
}

}
