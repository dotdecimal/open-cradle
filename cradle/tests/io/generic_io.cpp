#include <cradle/io/generic_io.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/shared_array.hpp>

#define BOOST_TEST_MODULE generic_io
#include <cradle/test.hpp>

using namespace cradle;

void test_value_io(value const& v)
{
    {
        write_value_file("value_file", v);
        value u;
        read_value_file(&u, "value_file");
        BOOST_CHECK_EQUAL(u, v);
    }

    {
        std::string s;
        value_to_string(&s, v);
        value u;
        parse_value_string(&u, s);
        BOOST_CHECK_EQUAL(u, v);
    }

    {
        std::string json;
        value_to_json(&json, v);
        value u;
        parse_json_value(&u, json);
        BOOST_CHECK_EQUAL(u, v);
    }

    {
        byte_vector data;
        serialize_value(&data, v);
        value u;
        deserialize_value(&u, &data[0], data.size());
        BOOST_CHECK_EQUAL(u, v);
    }

    {
        std::string s;
        value_to_base64_string(&s, v);
        value u;
        parse_base64_value_string(&u, s);
        BOOST_CHECK_EQUAL(u, v);

        // Corrupt the encoded string and check that the CRC test fails.
        char& c = s[s.length() - 3];
        if (c == '_')
            c = '-';
        else
            c = '_';
        BOOST_CHECK_THROW(parse_base64_value_string(&u, s),
            std::exception);
    }
}

static blob make_blob(size_t size)
{
    blob blob;
    blob.size = size;
    boost::shared_array<cradle::uint8_t> storage(new cradle::uint8_t[size]);
    blob.ownership = storage;
    for (std::size_t i = 0; i != size; ++i)
        storage[i] = cradle::uint8_t(i + 1);
    blob.data = storage.get();
    return blob;
}

BOOST_AUTO_TEST_CASE(generic_io_test)
{
    value t(true);
    test_value_io(t);
    value f(false);
    test_value_io(f);

    value i(number(1));
    test_value_io(i);
    value j(number(2));
    test_value_io(j);

    value x(0.1);
    test_value_io(x);
    value y(0.2);
    test_value_io(y);

    value s("foo");
    test_value_io(s);
    set(s, "bar");
    test_value_io(s);
    set(s, "");
    test_value_io(s);

    value b(make_blob(1000));
    test_value_io(b);
    set(b, make_blob(10));
    test_value_io(b);

    value_map r;
    test_value_io(value(r));
    r["b"] = b;
    test_value_io(value(r));
    r["x"] = x;
    test_value_io(value(r));
    r["y"] = y;
    test_value_io(value(r));

    value_list l;
    test_value_io(value(l));
    l.push_back(value(r));
    test_value_io(value(l));
    l.push_back(x);
    test_value_io(value(l));
    l.push_back(y);
    test_value_io(value(l));

    // Test the JSON output to make sure that the JSON I/O functions are
    // actually dealing in JSON.
    string json;
    value_to_json(&json, value(l));
    BOOST_CHECK_EQUAL(json,
        "[\n"
        "   {\n"
        "      \"b\" : {\n"
        "         \"blob\" : \"AQIDBAUGBwgJCg==\",\n"
        "         \"type\" : \"base64-encoded-blob\"\n"
        "      },\n"
        "      \"x\" : 0.10,\n"
        "      \"y\" : 0.20\n"
        "   },\n"
        "   0.10,\n"
        "   0.20\n"
        "]\n");
}
