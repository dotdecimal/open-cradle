#include <cradle/diff.hpp>

namespace cradle {

value_diff_item static
make_insert_item(value_diff_path const& path, value const& new_value)
{
    value_diff_item item;
    item.path = path;
    item.op = value_diff_op::INSERT;
    item.val = new_value;
    return item;
}

value_diff_item static
make_update_item(value_diff_path const& path, value const& new_value)
{
    value_diff_item item;
    item.path = path;
    item.op = value_diff_op::UPDATE;
    item.val = new_value;
    return item;
}

value_diff_item static
make_delete_item(value_diff_path const& path)
{
    value_diff_item item;
    item.path = path;
    item.op = value_diff_op::DELETE;
    return item;
}

value_diff_path static
extend_path(value_diff_path const& path, value const& addition)
{
    value_diff_path extended = path;
    extended.push_back(addition);
    return extended;
}

void static
compute_value_diff(value_diff& diff, value_diff_path const& path,
    value const& a, value const& b);

void static
compute_map_diff(value_diff& diff, value_diff_path const& path,
    value_map const& a, value_map const& b)
{
    auto a_i = a.begin(), a_end = a.end();
    auto b_i = b.begin(), b_end = b.end();
    while (1)
    {
        if (a_i != a_end)
        {
            if (b_i != b_end)
            {
                if (a_i->first == b_i->first)
                {
                    compute_value_diff(diff,
                        extend_path(path, a_i->first),
                        a_i->second, b_i->second);
                    ++a_i; ++b_i;
                }
                else if (a_i->first < b_i->first)
                {
                    diff.push_back(make_delete_item(
                        extend_path(path, a_i->first)));
                    ++a_i;
                }
                else
                {
                    diff.push_back(make_insert_item(
                        extend_path(path, b_i->first),
                        b_i->second));
                    ++b_i;
                }
            }
            else
            {
                diff.push_back(make_delete_item(
                    extend_path(path, a_i->first)));
                ++a_i;
            }
        }
        else
        {
            if (b_i != b_end)
            {
                diff.push_back(make_insert_item(
                    extend_path(path, b_i->first),
                    b_i->second));
                ++b_i;
            }
            else
                break;
        }
    }
}

struct insertion_description
{
    // index at which items were inserted
    size_t index;
    // number of items inserted
    size_t count;

    insertion_description() {}
    insertion_description(size_t index, size_t count)
      : index(index), count(count)
    {}
};

optional<insertion_description>
detect_insertion(value_list const& a, value_list const& b)
{
    size_t a_size = a.size();
    size_t b_size = b.size();
    assert(b_size > a_size);

    // Look for a mistmatch between the items in a and b.
    optional<size_t> removal_point;
    for (size_t i = 0; i != a_size; ++i)
    {
        if (a[i] != b[i])
        {
            // Scan through the remaining items in b looking for one that
            // matches a[i].
            size_t offset = 1;
            for (; i + offset != b_size; ++offset)
            {
                if (a[i] == b[i + offset])
                    goto found_match;
            }
            // None was found, so this wasn't an insertion.
            return none;
           found_match:
            // Now check that the remaining items in b match the remaining
            // items in a.
            for (size_t j = i; j != a_size; ++j)
            {
                // If we find another mismatch, give up.
                if (a[j] != b[j + offset])
                    return none;
            }
            return insertion_description(i, offset);
        }
    }

    // We didn't find a mismatch, so the last items were the ones inserted.
    return insertion_description(a_size, b_size - a_size);
}

void static
compute_list_diff(value_diff& diff, value_diff_path const& path,
    value_list const& a, value_list const& b)
{
    // For lists, we detect three common cases for compression:
    // * one or more items were inserted somewhere in the list
    // * one or more items were removed from the list
    // * the list didn't change size but items may have been updated
    // Any other case (or any combination of the above) is sent as an update
    // of the entire list.

    size_t a_size = a.size();
    size_t b_size = b.size();

    // Check if items were inserted.
    if (a_size < b_size)
    {
        auto insertion = detect_insertion(a, b);
        if (insertion)
        {
            for (size_t i = 0; i != get(insertion).count; ++i)
            {
                diff.push_back(
                    make_insert_item(
                        extend_path(path, to_value(get(insertion).index + i)),
                        b[get(insertion).index + i]));
            }
            return;
        }
    }
    // Check if an item was removed.
    else if (a_size > b_size)
    {
        auto removal = detect_insertion(b, a);
        if (removal)
        {
            for (size_t i = get(removal).count; i != 0; --i)
            {
                diff.push_back(
                    make_delete_item(
                        extend_path(path,
                            to_value(get(removal).index + i - 1))));
            }
            return;
        }
    }
    // If the lists are the same size, just diff each item.
    else
    {
        assert(a_size == b_size);
        for (size_t i = 0; i != a_size; ++i)
        {
            compute_value_diff(diff, extend_path(path, to_value(i)),
                a[i], b[i]);
        }
        return;
    }

    // If none of the above worked, send an update of the whole list.
    diff.push_back(make_update_item(path, value(b)));
}

void static
compute_value_diff(value_diff& diff, value_diff_path const& path,
    value const& a, value const& b)
{
    if (a != b)
    {
        // If a and b are both records, do a field-by-field diff.
        if (a.type() == value_type::MAP && b.type() == value_type::MAP)
        {
            compute_map_diff(diff, path,
                cast<value_map>(a), cast<value_map>(b));
        }
        // If a and b are both lists, do an item-by-item diff.
        else if (a.type() == value_type::LIST && b.type() == value_type::LIST)
        {
            compute_list_diff(diff, path,
                cast<value_list>(a), cast<value_list>(b));
        }
        // Otherwise, there's no way to compress the change, so just add an
        // update to the new value.
        else
        {
            diff.push_back(make_update_item(path, b));
        }
    }
}

value_diff compute_value_diff(value const& a, value const& b)
{
    value_diff diff;
    compute_value_diff(diff, value_diff_path(), a, b);
    return diff;
}

value static
apply_value_diff_item(value const& initial, value_diff_path const& path,
    size_t path_index, value_diff_op op, value const& new_value)
{
    size_t path_size = path.size();

    // If the path is empty, then we must be replacing the whole value.
    if (path_index == path_size)
        return new_value;

    // Otherwise, check out the next path element.
    auto const& path_element = path[path_index];
    switch (path_element.type())
    {
     case value_type::STRING:
      {
        if (initial.type() != value_type::MAP)
            throw invalid_diff_path();
        value_map map = cast<value_map>(initial);
        // If this is the last element, we need to actually act on it.
        if (path_index + 1 == path_size)
        {
            switch (op)
            {
             case value_diff_op::INSERT:
             case value_diff_op::UPDATE:
                map[path_element] = new_value;
                break;
             case value_diff_op::DELETE:
                map.erase(path_element);
                break;
            }
        }
        // Otherwise, just continue on down the path.
        else
        {
            auto field = map.find(path_element);
            if (field == map.end())
                throw invalid_diff_path();
            field->second =
                apply_value_diff_item(field->second, path, path_index + 1, op,
                    new_value);
        }
        return value(map);
      }
     case value_type::INTEGER:
      {
        if (initial.type() != value_type::LIST)
            throw invalid_diff_path();
        value_list list = cast<value_list>(initial);
        size_t index;
        from_value(&index, path_element);
        // If this is the last element, we need to actually act on it.
        if (path_index + 1 == path_size)
        {
            switch (op)
            {
             case value_diff_op::INSERT:
                if (index > list.size())
                    throw invalid_diff_path();
                list.insert(list.begin() + index, new_value);
             case value_diff_op::UPDATE:
                if (index >= list.size())
                    throw invalid_diff_path();
                list[index] = new_value;
                break;
             case value_diff_op::DELETE:
                list.erase(list.begin() + index);
                break;
            }
        }
        // Otherwise, just continue on down the path.
        else
        {
            if (index >= list.size())
                throw invalid_diff_path();
            list[index] =
                apply_value_diff_item(list[index], path, path_index + 1, op,
                    new_value);
        }
        return value(list);
      }
     default:
        throw invalid_diff_path();
    }
}

value apply_value_diff(value const& v, value_diff const& diff)
{
    value patched = v;
    for (auto const& item : diff)
    {
        patched = apply_value_diff_item(patched, item.path, 0, item.op,
            item.val ? get(item.val) : value());
    }
    return patched;
}

}
