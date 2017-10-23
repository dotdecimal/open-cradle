#include <cradle/encoding.hpp>
#include <boost/scoped_array.hpp>
#include <sstream>
#include <cradle/math/common.hpp>
#include <iomanip>

namespace cradle {

size_t get_base64_encoded_length(size_t raw_length)
{
    return (raw_length + 2) / 3 * 4 + 1;
}

size_t get_base64_decoded_length(size_t encoded_length)
{
    return (encoded_length + 3) / 4 * 3;
}

void base64_encode(
    char* dst, size_t* dst_size, uint8_t const* src, size_t src_size,
    base64_character_set const& character_set)
{
    uint8_t const* src_end = src + src_size;
    char const* dst_start = dst;
    while (1)
    {
        if (src == src_end)
            break;
        int n = *src;
        ++src;
        *dst++ = character_set.digits[(n >> 2) & 63];
        n <<= 8;
        if (src != src_end)
            n |= *src;
        *dst++ = character_set.digits[(n >> 4) & 63];
        if (src == src_end)
        {
            *dst++ = character_set.padding;
            *dst++ = character_set.padding;
            break;
        }
        ++src;
        n <<= 8;
        if (src != src_end)
            n |= *src;
        *dst++ = character_set.digits[(n >> 6) & 63];
        if (src == src_end)
        {
            *dst++ = character_set.padding;
            break;
        }
        ++src;
        *dst++ = character_set.digits[(n >> 0) & 63];
    }
    *dst = 0;
    *dst_size = dst - dst_start;
}

string base64_encode(uint8_t const* src, size_t src_size,
    base64_character_set const& character_set)
{
    boost::scoped_array<char> dst(
        new char[get_base64_encoded_length(src_size)]);
    size_t dst_size;
    base64_encode(dst.get(), &dst_size, src, src_size, character_set);
    return string(dst.get());
}

void base64_decode(
    uint8_t* dst, size_t* dst_size, char const* src, size_t src_size,
    base64_character_set const& character_set)
{
    uint8_t reverse_mapping[0x100];
    for (int i = 0; i != 0x100; ++i)
        reverse_mapping[i] = 0xff;
    for (uint8_t i = 0; i != 64; ++i)
        reverse_mapping[uint8_t(character_set.digits[i])] = i;

    char const* src_end = src + src_size;
    uint8_t const* dst_start = dst;

    while (1)
    {
        if (src == src_end)
            break;

        uint8_t c0 = reverse_mapping[uint8_t(*src)];
        ++src;
        if (c0 > 63 || src == src_end)
            throw exception("invalid base-64 string");

        uint8_t c1 = reverse_mapping[uint8_t(*src)];
        ++src;
        if (c1 > 63)
            throw exception("invalid base-64 string");

        *dst++ = (c0 << 2) | (c1 >> 4);

        if (src == src_end || *src == character_set.padding)
            break;

        uint8_t c2 = reverse_mapping[uint8_t(*src)];
        ++src;
        if (c2 > 63)
            throw exception("invalid base-64 string");

        *dst++ = ((c1 & 0xf) << 4) | (c2 >> 2);

        if (src == src_end || *src == character_set.padding)
            break;

        uint8_t c3 = reverse_mapping[uint8_t(*src)];
        ++src;
        if (c3 > 63)
            throw exception("invalid base-64 string");

        *dst++ = ((c2 & 0x3) << 6) | c3;
    }
    *dst_size = dst - dst_start;
}

static void base36_encode(
    std::ostringstream& s, uint64_t id, unsigned minimum_length)
{
    if (id != 0 || minimum_length != 0)
    {
        base36_encode(s, id / 36, minimum_length > 0 ? minimum_length - 1 : 0);
        char digit = char(id % 36);
        if (digit < 10)
            s.put('0' + digit);
        else
            s.put('a' + (digit - 10));
    }
}

string base36_encode(uint64_t id, unsigned minimum_length)
{
    std::ostringstream s;
    base36_encode(s, id, minimum_length);
    return s.str();
}

uint64_t base36_decode(string const& text)
{
    size_t n_digits = text.length();

    // Technically, some 13-digit strings are valid base-36, but we'll never
    // get anywhere near that length.
    if (n_digits < 1 || n_digits > 12)
        throw exception("invalid base-36 string");

    uint64_t id = 0;
    for (size_t i = 0; i != n_digits; ++i)
    {
        id *= 36;
        char c = text[i];
        if (c >= '0' && c <= '9')
            id += c - '0';
        else if (c >= 'a' && c <= 'z')
            id += 10 + (c - 'a');
        else if (c >= 'A' && c <= 'Z')
            id += 10 + (c - 'A');
        else
            throw exception("invalid base-36 string");
    }
    return id;
}

static unsigned base36_digits_required(uint64_t id)
{
    uint64_t range = 1;
    unsigned n_digits = 0;
    while (range <= id)
    {
        range *= 36;
        ++n_digits;
    }
    return n_digits;
}

static uint64_t scramble_id(uint64_t id, unsigned n_digits)
{
    uint64_t half_range = 1;
    for (unsigned i = 0; i != n_digits; ++i)
        half_range *= 6;
    uint64_t l = id / half_range, r = id % half_range;
    for (unsigned i = 0; i != 6; ++i)
    {
        uint64_t r_next = uint64_t(
            nonnegative_mod(int64_t(l) + (int64_t(r) * 235 + 1),
                int64_t(half_range)));
        l = r;
        r = r_next;
    }
    return l * half_range + r;
}
static uint64_t descramble_id(uint64_t id, unsigned n_digits)
{
    uint64_t half_range = 1;
    for (unsigned i = 0; i != n_digits; ++i)
        half_range *= 6;
    uint64_t l = id / half_range, r = id % half_range;
    for (unsigned i = 0; i != 6; ++i)
    {
        uint64_t l_next = uint64_t(
            nonnegative_mod(int64_t(r) - (int64_t(l) * 235 + 1),
                int64_t(half_range)));
        r = l;
        l = l_next;
    }
    return l * half_range + r;
}

string nonsequential_base36_encode(uint64_t id, unsigned minimum_length)
{
    unsigned n_digits = (std::max)(minimum_length, base36_digits_required(id));
    return base36_encode(scramble_id(id, n_digits), n_digits);
}

uint64_t nonsequential_base36_decode(string const& text)
{
    unsigned n_digits = unsigned(text.length());
    return descramble_id(base36_decode(text), n_digits);
}

string ascii_to_hex(unsigned char * text, int length)
{
    std::string retString;
    std::ostringstream stream;
    for (int i = 0; i < length; i++)
    {
        stream << std::setfill('0') << std::setw(2) << std::hex << int(text[i]);
    }

    retString = stream.str();
    return retString;
}

}
