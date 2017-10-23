#ifndef CRADLE_ENCODING_HPP
#define CRADLE_ENCODING_HPP

#include <string>
#include <cradle/common.hpp>

namespace cradle {

// BASE-64

// This section provides functions for converting raw binary data to and from
// a base-64 ASCII encoding.  The characters used to represent the 64 values
// are specified as a function parameter.  Two obvious choices are provided
// here: the MIME character set, and a modified MIME set that is URL-friendly.

// Given the length of a raw binary sequence, this gives the maximum length of
// the corresponding base64 encoding, including the terminating null character.
// The actual length of the encoded sequence may be shorter by one or two
// characters.
size_t get_base64_encoded_length(size_t raw_length);

// Given the length of a base64 encoded sequence, this gives the maximum
// length of corresponding decoded binary sequence.  The actual length of
// the decoded sequence may be shorter by one or two bytes.
size_t get_base64_decoded_length(size_t encoded_length);

struct base64_character_set
{
    // the 64 digits used to represent the 6-bit values
    char const* digits;
    // the character used to pad the sequence to a multiple of 4 characters
    // Set this to 0 for no padding.
    char padding;
};

static inline
base64_character_set get_mime_base64_character_set()
{
    base64_character_set set;
    set.digits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    set.padding = '=';
    return set;
};

static inline
base64_character_set get_url_friendly_base64_character_set()
{
    base64_character_set set;
    set.digits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    set.padding = 0;
    return set;
};

// Encode the given source data in base64 using the provided character set.
// dst must be able to hold at least get_base64_encoded_length(src_size) chars.
// *dst_size will be set to the actual length of the encoded string, not
// including the terminating null character.
void base64_encode(
    char* dst, size_t* dst_size, uint8_t const* src, size_t src_size,
    base64_character_set const& character_set);

// Calls the above and returns the result as a string.
string base64_encode(uint8_t const* src, size_t src_size,
    base64_character_set const& character_set);

// Decode the given base64 data using the provided character set.
// dst must be able to hold at least get_base64_decoded_length(src_size) bytes.
// *dst_size will be set to the actual length of the decoded data.
void base64_decode(
    uint8_t* dst, size_t* dst_size, char const* src, size_t src_size,
    base64_character_set const& character_set);

// BASE-36

// The following provides base-36 encoding and decoding of 64-bit integers.
// The primary use of base-36 is as a quick and accurate way for humans to deal
// with ID numbers. (It requires fewer digits than decimal, but it avoids the
// confusion of case sensitivity and extra characters that might be introduced
// by base-64.)

// minimum_length is the minimum number of digits in the output string.
string base36_encode(uint64_t id, unsigned minimum_length = 1);

uint64_t base36_decode(string const& text);

// A desirable property of encoded IDs is that they appear nonsequential, so
// that consecutive IDs (which are likely in use concurrently) are as distinct
// as possible. The following functions implement a base-36 encoding with this
// property.

// minimum_length is the minimum number of digits in the output string.
string nonsequential_base36_encode(
    uint64_t id, unsigned minimum_length = 1);

uint64_t nonsequential_base36_decode(string const& text);

// BASE-16

// The following provides the conversion from a string of characters to its
// corresponding base-16 encoded string.

string ascii_to_hex(unsigned char * text, int length);

}

#endif
