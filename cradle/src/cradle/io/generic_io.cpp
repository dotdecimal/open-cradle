#include <cradle/io/generic_io.hpp>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/shared_array.hpp>

#include <json/json.h>

#include <cradle/date_time.hpp>
#include <cradle/encoding.hpp>
#include <cradle/io/compression.hpp>
#include <cradle/io/crc.hpp>
#include <cradle/io/file.hpp>
#include <cradle/io/msgpack_io.hpp>
#include <cradle/io/raw_memory_io.hpp>
#include <cradle/io/text_parser.hpp>

namespace cradle {

// This is used in various places for datetime I/O.
static boost::posix_time::ptime const the_epoch(boost::gregorian::date(1970, 1, 1));

// STRING I/O

// Using JSON as the string representation for now.
void parse_value_string(value* v, string const& s)
{
    return parse_json_value(v, s);
}
void value_to_string(string* s, value const& v)
{
    value_to_json(s, v);
}

blob string_to_blob(string const& s)
{
    blob blob;
    // Don't include the terminating '\0'.
    alia__shared_ptr<char> ptr(
        new char[s.length()], array_deleter<char>());
    blob.ownership = ptr;
    blob.data = ptr.get();
    memcpy(ptr.get(), s.c_str(), s.length());
    blob.size = s.length();
    return blob;
}

// JSON I/O

// Check if a JSON list is actually an encoded map.
// This is the case if the list contains only key/value pairs.
bool static
list_resembles_map(Json::Value const& json)
{
    assert(json.type() == Json::arrayValue);
    Json::Value::ArrayIndex n_elements = json.size();
    for (Json::Value::ArrayIndex i = 0; i != n_elements; ++i)
    {
        auto const& element = json[i];
        if (element.type() != Json::objectValue || element.size() != 2 ||
            !element.isMember("key") || !element.isMember("value"))
        {
            return false;
        }
    }
    return true;
}

// Read a JSON value into a CRADLE value.
void static
read_json_value(
    value* v,
    check_in_interface& check_in,
    Json::Value const& json)
{
    switch (json.type())
    {
     case Json::nullValue:
        set(*v, nil);
        break;
     case Json::intValue:
        set(*v, integer(json.asInt()));
        break;
     case Json::uintValue:
        set(*v, integer(json.asUInt()));
        break;
     case Json::realValue:
        set(*v, double(json.asDouble()));
        break;
     case Json::stringValue:
      {
        auto s = json.asString();
        // Times are also encoded as JSON strings, so this checks to see if
        // the string parses as a time. If so, it just assumes it's actually a
        // time.
        // First check if it looks anything like a time string.
        if (s.length() > 16 &&
            isdigit(s[0]) &&
            isdigit(s[1]) &&
            isdigit(s[2]) &&
            isdigit(s[3]) &&
            s[4] == '-')
        {
            try
            {
                auto t = parse_time(s);
                // Check that it can be converted back without changing its
                // value. This could be necessary if we actually expected a
                // string here.
                if (to_value_string(t) == s)
                {
                    set(*v, parse_time(s));
                    break;
                }
            }
            catch (...)
            {
            }
        }
        set(*v, s);
        break;
      }
     case Json::booleanValue:
        set(*v, json.asBool());
        break;
     case Json::arrayValue:
      {
        Json::Value::ArrayIndex n_elements = json.size();
        // If this resembles an encoded map, read it as that.
        if (!json.empty() && list_resembles_map(json))
        {
            value_map map;
            for (Json::Value::ArrayIndex i = 0; i != n_elements; ++i)
            {
                auto const& element = json[i];
                value k;
                read_json_value(&k, check_in, element["key"]);
                value v;
                read_json_value(&v, check_in, element["value"]);
                map[k] = v;
            }
            v->swap_in(map);
        }
        // Otherwise, read it as an actual list.
        else
        {
            value_list values(n_elements);
            for (Json::Value::ArrayIndex i = 0; i != n_elements; ++i)
                read_json_value(&values[i], check_in, json[i]);
            v->swap_in(values);
        }
        break;
      }
     case Json::objectValue:
      {
        // An object is analagous to a record, but blobs and references
        // are also encoded as JSON objects, so we have to check here if
        // it's actually one of those.
        Json::Value const& type = json["type"];
        if (type.type() == Json::stringValue &&
            type.asString() == "base64-encoded-blob")
        {
            Json::Value const& json_blob = json["blob"];
            if (json_blob.type() == Json::stringValue)
            {
                string encoded = json_blob.asString();
                blob x;
                text_parser p;
                initialize(p, encoded, encoded.c_str(), encoded.length());
                size_t decoded_size =
                    get_base64_decoded_length(encoded.length());
                alia__shared_ptr<uint8_t> ptr(new uint8_t[decoded_size],
                    array_deleter<uint8_t>());
                x.ownership = ptr;
                x.data = reinterpret_cast<void const*>(ptr.get());
                base64_decode(ptr.get(), &x.size, encoded.c_str(),
                    encoded.length(), get_mime_base64_character_set());
                set(*v, x);
                break;
            }
            throw json_parse_error(
                "incorrectly formatted base64-encoded-blob");
        }
        // Otherwise, interpret it as a record.
        value_map map;
        Json::Value::Members members = json.getMemberNames();
        for (Json::Value::Members::const_iterator i = members.begin();
            i != members.end(); ++i)
        {
            read_json_value(&map[value(*i)], check_in,
                json.get(*i, Json::Value::null));
        }
        v->swap_in(map);
        break;
      }
    }
}

void parse_json_value(value* v, char const* json, size_t length)
{
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(json, json + length, root))
        throw json_parse_error(reader.getFormatedErrorMessages());
    null_check_in check_in;
    read_json_value(v, check_in, root);
}

bool static
has_only_string_keys(value_map const& map)
{
    for (auto const& i : map)
    {
        if (i.first.type() != value_type::STRING)
            return false;
    }
    return true;
}

static void write_json_value(Json::Value* json, value const& v)
{
    switch (v.type())
    {
     case value_type::NIL:
        *json = Json::Value::null;
        break;
     case value_type::BOOLEAN:
        *json = cast<bool>(v);
        break;
     case value_type::INTEGER:
        *json = double(cast<integer>(v));
        break;
     case value_type::FLOAT:
        *json = cast<double>(v);
        break;
     case value_type::STRING:
        *json = cast<string>(v);
        break;
     case value_type::BLOB:
      {
        blob const& x = cast<blob>(v);
        (*json)["type"] = "base64-encoded-blob";
        (*json)["blob"] =
            base64_encode(static_cast<uint8_t const*>(x.data), x.size,
                get_mime_base64_character_set());
        break;
      }
     case value_type::DATETIME:
      {
        *json = to_value_string(cast<boost::posix_time::ptime>(v));
        break;
      }
     case value_type::LIST:
      {
        *json = Json::Value(Json::arrayValue);
        value_list const& x = cast<value_list>(v);
        Json::Value::ArrayIndex size =
            boost::numeric_cast<Json::Value::ArrayIndex>(x.size());
        json->resize(size);
        for (Json::Value::ArrayIndex i = 0; i != size; ++i)
            write_json_value(&(*json)[i], x[i]);
        break;
      }
     case value_type::MAP:
      {
        value_map const& x = cast<value_map>(v);
        // If the map has only key strings, encode it directly as a JSON
        // object.
        if (has_only_string_keys(x))
        {
            *json = Json::Value(Json::objectValue);
            for (value_map::const_iterator i = x.begin(); i != x.end(); ++i)
                write_json_value(&(*json)[cast<string>(i->first)], i->second);
        }
        // Otherwise, encode it as a list of key/value pairs.
        else
        {
            *json = Json::Value(Json::arrayValue);
            Json::Value::ArrayIndex size =
                boost::numeric_cast<Json::Value::ArrayIndex>(x.size());
            json->resize(size);
            Json::Value::ArrayIndex index = 0;
            for (value_map::const_iterator i = x.begin(); i != x.end(); ++i)
            {
                write_json_value(&(*json)[index]["key"], i->first);
                write_json_value(&(*json)[index]["value"], i->second);
                ++index;
            }
        }
        break;
      }
    }
}

void value_to_json(string* json, value const& v)
{
    Json::Value root;
    write_json_value(&root, v);
    Json::StyledWriter writer;
    *json = writer.write(root);
}

blob value_to_json_blob(value const& v)
{
    string json = value_to_json(v);
    blob blob;
    // Don't include the terminating '\0'.
    alia__shared_ptr<char> ptr(
        new char[json.length()], array_deleter<char>());
    blob.ownership = ptr;
    blob.data = ptr.get();
    memcpy(ptr.get(), json.c_str(), json.length());
    blob.size = json.length();
    return blob;
}

// MSGPACK I/O

void static
read_msgpack_value(
    value* v,
    ownership_holder const& ownership,
    msgpack::object const& object)
{
    switch (object.type)
    {
     case msgpack::type::NIL:
        set(*v, nil);
        break;
     case msgpack::type::BOOLEAN:
        set(*v, object.via.boolean);
        break;
     case msgpack::type::POSITIVE_INTEGER:
        // Maybe we should check for overflow here.
        set(*v, integer(object.via.u64));
        break;
     case msgpack::type::NEGATIVE_INTEGER:
        set(*v, integer(object.via.i64));
        break;
     case msgpack::type::FLOAT:
        set(*v, double(object.via.f64));
        break;
     case msgpack::type::STR:
      {
        string x;
        object.convert(x);
        set(*v, x);
        break;
      }
     case msgpack::type::BIN:
      {
        blob b;
        b.ownership = ownership;
        b.size = object.via.bin.size;
        b.data = object.via.bin.ptr;
        set(*v, b);
        break;
      }
     case msgpack::type::ARRAY:
      {
        size_t n_elements = object.via.array.size;
        value_list list;
        list.resize(n_elements);
        for (size_t i = 0; i != n_elements; ++i)
            read_msgpack_value(&list[i], ownership, object.via.array.ptr[i]);
        v->swap_in(list);
        break;
      }
     case msgpack::type::MAP:
      {
        value_map map;
        for (size_t i = 0; i != object.via.map.size; ++i)
        {
            auto const& pair = object.via.map.ptr[i];
            value key;
            read_msgpack_value(&key, ownership, pair.key);
            read_msgpack_value(&map[key], ownership, pair.val);
        }
        v->swap_in(map);
        break;
      }
     case msgpack::type::EXT:
      {
        switch (object.via.ext.type())
        {
         case 1: // datetime
          {
            int64_t t;
            auto const* data = object.via.ext.data();
            switch (object.via.ext.size)
            {
             case 1:
                t = *reinterpret_cast<int8_t const*>(data);
                break;
             case 2:
              {
                uint16_t swapped =
                    swap_uint16_on_little_endian(
                        *reinterpret_cast<uint16_t const*>(data));
                t = *reinterpret_cast<int16_t const*>(&swapped);
                break;
              }
             case 4:
              {
                uint32_t swapped =
                    swap_uint32_on_little_endian(
                        *reinterpret_cast<uint32_t const*>(data));
                t = *reinterpret_cast<int32_t const*>(&swapped);
                break;
              }
             case 8:
              {
                uint64_t swapped =
                    swap_uint64_on_little_endian(
                        *reinterpret_cast<uint64_t const*>(data));
                t = *reinterpret_cast<int64_t const*>(&swapped);
                break;
              }
            }
            set(*v, the_epoch + boost::posix_time::milliseconds(t));
            break;
          }
         default:
            throw exception("unsupported MessagePack extension type");
        }
        break;
      }
    }
}

void parse_msgpack_value(value* v, uint8_t const* data, size_t size)
{
    // msgpack::unpack returns a unique handle which contains the object and
    // also owns the data stored within the object. Copying the handle
    // transfers ownership of the data.
    // We want to be able to capture the blobs in the object without copying
    // all their data, so in order to do that, we create a shared_ptr to the
    // object handle and pass that in as the ownership_holder for the blobs to
    // use.
    msgpack::object_handle handle =
        msgpack::unpack(reinterpret_cast<char const*>(data), size);
    alia__shared_ptr<msgpack::object_handle>
        shared_handle(new msgpack::object_handle);
    *shared_handle = handle;
    ownership_holder ownership;
    ownership = shared_handle;
    read_msgpack_value(v, ownership, shared_handle->get());
}

value parse_msgpack_value(string const& msgpack)
{
    value v;
    parse_msgpack_value(&v, reinterpret_cast<uint8_t const*> (msgpack.c_str()),
        msgpack.length());
    return v;
}

// This is passed to the msgpack unpacker to tell it whether different types
// of objects should be copied out of the packed buffer or referenced
// directly.
bool static
msgpack_unpack_reference_type(
    msgpack::type::object_type type,
    size_t length,
    void* user_data)
{
    // Reference blobs directly, but copy anything else.
    return type == msgpack::type::BIN;
}

void
parse_msgpack_value(
    value* v,
    ownership_holder const& ownership,
    uint8_t const* data,
    size_t size)
{
    msgpack::object_handle handle =
        msgpack::unpack(
            reinterpret_cast<char const*>(data),
            size,
            msgpack_unpack_reference_type);
    read_msgpack_value(v, ownership, handle.get());
}

string value_to_msgpack_string(value const& v)
{
    std::stringstream stream;
    msgpack::packer<std::stringstream> packer(stream);
    write_msgpack_value(packer, v);
    return stream.str();
}

blob value_to_msgpack_blob(value const& v)
{
    alia__shared_ptr<msgpack::sbuffer> sbuffer(new msgpack::sbuffer);
    msgpack::packer<msgpack::sbuffer> packer(*sbuffer);
    write_msgpack_value(packer, v);
    blob b;
    b.ownership = sbuffer;
    b.data = sbuffer->data();
    b.size = sbuffer->size();
    return b;
}

// MEMORY I/O

namespace {

void static
read_raw_value(raw_memory_reader& r, value& v)
{
    value_type type;
    {
        uint32_t t;
        raw_read(r, &t, 4);
        type = value_type(t);
    }
    switch (type)
    {
     case value_type::NIL:
        set(v, nil);
        break;
     case value_type::BOOLEAN:
      {
        uint8_t x;
        raw_read(r, &x, 1);
        set(v, bool(x != 0));
        break;
      }
     case value_type::INTEGER:
      {
        integer x;
        raw_read(r, &x, 8);
        set(v, x);
        break;
      }
     case value_type::FLOAT:
      {
        double x;
        raw_read(r, &x, 8);
        set(v, x);
        break;
      }
     case value_type::STRING:
      {
        set(v, read_string<uint32_t>(r));
        break;
      }
     case value_type::BLOB:
      {
        uint64_t length;
        raw_read(r, &length, 8);
        blob x;
        x.size = boost::numeric_cast<size_t>(length);
        alia__shared_ptr<uint8_t> ptr(new uint8_t[x.size],
            array_deleter<uint8_t>());
        x.ownership = ptr;
        x.data = reinterpret_cast<void const*>(ptr.get());
        raw_read(r, const_cast<void*>(x.data), x.size);
        set(v, x);
        break;
      }
     case value_type::DATETIME:
      {
        int64_t t;
        raw_read(r, &t, 8);
        set(v, the_epoch + boost::posix_time::milliseconds(t));
        break;
      }
     case value_type::LIST:
      {
        uint64_t length;
        raw_read(r, &length, 8);
        value_list value(boost::numeric_cast<size_t>(length));
        for (value_list::iterator i = value.begin(); i != value.end(); ++i)
            read_raw_value(r, *i);
        set(v, value);
        break;
      }
     case value_type::MAP:
      {
        uint64_t length;
        raw_read(r, &length, 8);
        value_map map;
        for (uint64_t i = 0; i != length; ++i)
        {
            value key;
            read_raw_value(r, key);
            value value;
            read_raw_value(r, value);
            map[key] = value;
        }
        set(v, map);
        break;
      }
    }
}

void static
read_raw_value(value* v, uint8_t const* data, size_t size)
{
    raw_memory_reader r(data, size);
    read_raw_value(r, *v);
}

void static
write_raw_value(raw_memory_writer& w, value const& v)
{
    {
        uint32_t t = uint32_t(v.type());
        raw_write(w, &t, 4);
    }
    switch (v.type())
    {
     case value_type::NIL:
        break;
     case value_type::BOOLEAN:
      {
        uint8_t t = cast<bool>(v) ? 1 : 0;
        raw_write(w, &t, 1);
        break;
      }
     case value_type::INTEGER:
      {
        integer t = cast<integer>(v);
        raw_write(w, &t, 8);
        break;
      }
     case value_type::FLOAT:
      {
        double t = cast<double>(v);
        raw_write(w, &t, 8);
        break;
      }
     case value_type::STRING:
        write_string<uint32_t>(w, cast<string>(v));
        break;
     case value_type::BLOB:
      {
        blob const& x = cast<blob>(v);
        uint64_t length = x.size;
        raw_write(w, &length, 8);
        raw_write(w, x.data, x.size);
        break;
      }
     case value_type::DATETIME:
      {
        int64_t t = (cast<time>(v) - the_epoch).total_milliseconds();
        raw_write(w, &t, 8);
        break;
      }
     case value_type::LIST:
      {
        value_list const& x = cast<value_list>(v);
        uint64_t size = x.size();
        raw_write(w, &size, 8);
        for (value_list::const_iterator i = x.begin(); i != x.end(); ++i)
            write_raw_value(w, *i);
        break;
      }
     case value_type::MAP:
      {
        value_map const& x = cast<value_map>(v);
        uint64_t size = x.size();
        raw_write(w, &size, 8);
        for (value_map::const_iterator i = x.begin(); i != x.end(); ++i)
        {
            write_raw_value(w, i->first);
            write_raw_value(w, i->second);
        }
        break;
      }
    }
}

void static
write_raw_value(byte_vector* data, value const& v)
{
    raw_memory_writer w(*data);
    write_raw_value(w, v);
}

}

// CHECKED MEMORY I/O

namespace {

// Get the length of a number stored as a 0xff-terminated base-255 string.
size_t base_255_length(uint64_t n)
{
    size_t length = 1;
    while (n > 0)
    {
        n /= 255;
        ++length;
    }
    return length;
}

// Write a number as a 0xff-terminated base-255 string.
// 'length' is the length of the string, as computed above.
void write_base_255_number(uint8_t* data, size_t length, uint64_t n)
{
    data += length - 1;
    *data = 0xff;
    --data;
    while (n > 0)
    {
        *data = (n % 255);
        --data;
        n /= 255;
    }
}

// Read a number as a 0xff-terminated base-255 string.
uint64_t read_base_255_number(uint8_t const*& data, size_t& size)
{
    uint64_t n = 0;
    while (1)
    {
        if (size == 0)
            throw corrupt_data();
        uint8_t digit = *data;
        ++data;
        --size;
        if (digit == 0xff)
            break;
        n *= 255;
        n += digit;
    }
    return n;
}

}

void deserialize_value(value* v, uint8_t const* data, size_t size,
    uint32_t* crc)
{
    size_t const crc_size = 4;
    uint32_t recorded_crc;
    std::memcpy(&recorded_crc, data, crc_size);
    data += crc_size;
    size -= crc_size;

    size_t raw_size =
        boost::numeric_cast<size_t>(read_base_255_number(data, size));

    boost::scoped_array<uint8_t> raw(new uint8_t[raw_size]);
    decompress(raw.get(), raw_size, data, size);

    uint32_t computed_crc = compute_crc32(0, raw.get(), raw_size);
    if (recorded_crc != computed_crc)
        throw crc_error();
    if (crc)
        *crc = computed_crc;

    read_raw_value(v, raw.get(), raw_size);
}

void serialize_value(byte_vector* data, value const& v, uint32_t* crc)
{
    byte_vector raw;
    write_raw_value(&raw, v);
    size_t raw_size = raw.size();

    uint32_t computed_crc = compute_crc32(0, &raw[0], raw_size);
    if (crc)
        *crc = computed_crc;

    boost::scoped_array<uint8_t> compressed;
    size_t compressed_size;
    compress(&compressed, &compressed_size, &raw[0], raw_size);

    // We no longer need the memory used to hold the raw bytes, so free it.
    {
        byte_vector tmp;
        raw.swap(tmp);
    }

    size_t const crc_size = 4;
    size_t const size_size = base_255_length(raw_size);
    data->resize(crc_size + size_size + compressed_size);
    // TODO: All this copying could be eliminated with better interfaces, but
    // the time it takes to serialize data is less important than the time it
    // takes to deserialize, so the effort required to redesign the interfaces
    // doesn't seem worth it.
    uint8_t* p = &(*data)[0];
    std::memcpy(p, &computed_crc, crc_size);
    p += crc_size;
    write_base_255_number(p, size_size, raw_size);
    p += size_size;
    std::memcpy(p, compressed.get(), compressed_size);
}

// FILE I/O

static void write_block(std::ofstream& f, void const* data, size_t size)
{
    // Windows has demonstrated problems handling large blocks of data,
    // so we split them up before passing them in.
    char const* p = reinterpret_cast<char const*>(data);
    size_t max_block_size = 40000000;
    while (size > max_block_size)
    {
        f.write(p, max_block_size);
        p += max_block_size;
        size -= max_block_size;
    }
    f.write(p, size);
}

static void read_block(std::ifstream& f, void* data, size_t size)
{
    // Analagous to write_block().
    char* p = reinterpret_cast<char*>(data);
    size_t max_block_size = 40000000;
    while (size > max_block_size)
    {
        f.read(p, max_block_size);
        p += max_block_size;
        size -= max_block_size;
    }
    f.read(p, size);
}

void read_value_file(value* v, file_path const& file, uint32_t* crc)
{
    // TODO: 64-bit proof this
    std::ifstream f;
    open(f, file, std::ios::in | std::ios::binary);
    byte_vector raw;
    f.seekg(0, std::ios::end);
    // TODO: actual 64-bit alternative
    size_t size = boost::numeric_cast<size_t>(int64_t(f.tellg()));
    f.seekg(0);
    raw.resize(size);
    read_block(f, &raw[0], size);
    deserialize_value(v, &raw[0], size, crc);
}

void write_value_file(file_path const& file, value const& v, uint32_t* crc)
{
    byte_vector raw;
    serialize_value(&raw, v, crc);
    std::ofstream f;
    open(f, file, std::ios::out | std::ios::binary | std::ios::trunc);
    write_block(f, &raw[0], raw.size());
}

// BASE-64 I/O

void parse_base64_value_string(value* v, string const& s, uint32_t* crc)
{
    byte_vector raw;
    raw.resize(get_base64_decoded_length(s.length()));
    size_t raw_size;
    base64_decode(&raw[0], &raw_size, s.c_str(), s.length(),
        get_url_friendly_base64_character_set());
    deserialize_value(v, &raw[0], raw_size, crc);
}
void value_to_base64_string(string* s, value const& v, uint32_t* crc)
{
    byte_vector raw;
    serialize_value(&raw, v, crc);
    *s = base64_encode(&raw[0], raw.size(), get_url_friendly_base64_character_set());
}

void initialize_parser_with_file(text_parser& p, filesystem_item const& item)
{
    if (item.contents.type != filesystem_item_contents_type::FILE)
        throw exception("filesystem directory used where a file was expected");
    initialize_parser_with_blob(p, item.name, as_file(item.contents));
}

}
