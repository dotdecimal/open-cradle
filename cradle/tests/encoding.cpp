#include <cradle/encoding.hpp>
#include <boost/scoped_array.hpp>
#include <cstdlib>
#include <boost/algorithm/string.hpp>
#include <set>

#define BOOST_TEST_MODULE base64
#include <cradle/test.hpp>

using namespace cradle;

static void check_mime_encoding(char const* original, char const* encoded)
{
    using namespace cradle;
    BOOST_CHECK_EQUAL(
        base64_encode(reinterpret_cast<cradle::uint8_t const*>(original),
            strlen(original), mime_base64_character_set),
        encoded);
}

static void check_base64_round_trip(std::vector<cradle::uint8_t> const& src,
    base64_character_set const& character_set)
{
    string encoded = base64_encode(&src[0], src.size(), character_set);
    std::vector<cradle::uint8_t> decoded(
        get_base64_decoded_length(encoded.length()));
    size_t decoded_length;
    base64_decode(&decoded[0], &decoded_length, encoded.c_str(),
        encoded.length(), character_set);
    decoded.resize(decoded_length);
    CRADLE_CHECK_RANGES_EQUAL(src, decoded);
}

static void test_random_base64_encoding(
    base64_character_set const& character_set)
{
    for (int i = 0; i != 100; ++i)
    {
        size_t length = rand() & 0xfff;
        std::vector<cradle::uint8_t> data(length);
        for (size_t j = 0; j != length; ++j)
            data[j] = cradle::uint8_t(rand() & 0xff);
        check_base64_round_trip(data, character_set);
    }
}

BOOST_AUTO_TEST_CASE(base64_encoding_test)
{
    check_mime_encoding(
        "Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure.",
        "TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2YgdGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGludWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRoZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=");

    check_mime_encoding(
        "leasure.",
        "bGVhc3VyZS4=");

    check_mime_encoding(
        "easure.",
        "ZWFzdXJlLg==");

    check_mime_encoding(
        "asure.",
        "YXN1cmUu");

    check_mime_encoding(
        "sure.",
        "c3VyZS4=");

    check_mime_encoding(
        "sure",
        "c3VyZQ==");

    test_random_base64_encoding(url_friendly_base64_character_set);
    test_random_base64_encoding(mime_base64_character_set);
}

static void check_base36_round_trip(
    uint64_t id, unsigned minimum_length, string const& correct_encoding)
{
    string encoded = base36_encode(id, minimum_length);
    BOOST_CHECK_EQUAL(encoded, correct_encoding);
    uint64_t decoded = base36_decode(encoded);
    BOOST_CHECK_EQUAL(id, decoded);
    decoded = base36_decode(boost::to_upper_copy(encoded));
    BOOST_CHECK_EQUAL(id, decoded);
}

static void check_invalid_base36(string const& text)
{
    BOOST_CHECK_THROW(base36_decode(text), exception);
}

BOOST_AUTO_TEST_CASE(base36_encoding_test)
{
    check_base36_round_trip(   0, 1,   "0");
    check_base36_round_trip(   0, 2,  "00");
    check_base36_round_trip(   1, 2,  "01");
    check_base36_round_trip(  10, 1,   "a");
    check_base36_round_trip(  35, 1,   "z");
    check_base36_round_trip(  36, 2,  "10");
    check_base36_round_trip(  36, 1,  "10");
    check_base36_round_trip(  71, 1,  "1z");
    check_base36_round_trip(1000, 1,  "rs");
    check_base36_round_trip(2000, 1, "1jk");
    check_invalid_base36("");
    check_invalid_base36("-");
    check_invalid_base36("1-");
    check_invalid_base36("/");
    check_invalid_base36(".");
    check_invalid_base36("0.");
    check_invalid_base36("a111111111111");
    check_invalid_base36("11111111111111");
}

static void check_nonsequential_base36_round_trip(
    uint64_t id, unsigned minimum_length, string const& correct_encoding)
{
    string encoded = nonsequential_base36_encode(id, minimum_length);
    BOOST_CHECK_EQUAL(encoded, correct_encoding);
    uint64_t decoded = nonsequential_base36_decode(encoded);
    BOOST_CHECK_EQUAL(id, decoded);
    decoded = nonsequential_base36_decode(boost::to_upper_copy(encoded));
    BOOST_CHECK_EQUAL(id, decoded);
}

static void check_nonsequential_base36_series(
    uint64_t n_ids, unsigned minimum_length)
{
    std::set<string> ids;
    for (uint64_t i = 0; i != n_ids; ++i)
    {
        string encoded = nonsequential_base36_encode(i, minimum_length);
        uint64_t decoded = nonsequential_base36_decode(encoded);
        BOOST_CHECK_EQUAL(i, decoded);
        ids.insert(encoded);
    }
    BOOST_CHECK_EQUAL(ids.size(), n_ids);
}

BOOST_AUTO_TEST_CASE(nonsequential_base36_encoding_test)
{
    // Check several IDs to make sure they indeed appear nonsequential.
    check_nonsequential_base36_round_trip(   0, 4, "0c4k");
    check_nonsequential_base36_round_trip(   1, 4, "4kqx");
    check_nonsequential_base36_round_trip(   2, 4, "8sda");
    check_nonsequential_base36_round_trip(   3, 4, "d0zn");
    check_nonsequential_base36_round_trip(   4, 4, "h8m0");
    check_nonsequential_base36_round_trip(   5, 4, "lg8d");
    check_nonsequential_base36_round_trip(   6, 4, "pouq");
    check_nonsequential_base36_round_trip(   7, 4, "twh3");
    check_nonsequential_base36_round_trip(   8, 4, "y43g");
    check_nonsequential_base36_round_trip(   9, 4, "2cpt");
    // Try a different area.
    check_nonsequential_base36_round_trip(1000, 3, "6qk");
    check_nonsequential_base36_round_trip(1001, 3, "w0x");
    check_nonsequential_base36_round_trip(1002, 3, "lha");
    check_nonsequential_base36_round_trip(1003, 3, "arn");
    check_nonsequential_base36_round_trip(1004, 3, "020");
    check_nonsequential_base36_round_trip(1005, 3, "pcd");

    // Check the first 200k IDs (with different minimum lengths for the IDs.
    // Ensure that all encodings decode back to the original ID and that no two
    // IDs map to the same encoding.
    check_nonsequential_base36_series(200000, 1);
    check_nonsequential_base36_series(200000, 3);
    check_nonsequential_base36_series(200000, 6);
}
