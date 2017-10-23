#ifndef CRADLE_RT_TEST_UPGRADES_HPP
#define CRADLE_RT_TEST_UPGRADES_HPP

#include <cradle/imaging/color.hpp>
#include <cradle/imaging/slicing.hpp>
#include <cradle/date_time.hpp>
#include <string>
#include <vector>

typedef unsigned short UINT16;

namespace cradle {


api(struct)
struct upgrade_test_structure
{
    // test string
    string test_structure_string;
    // added another test string to test upgrades
    string test_structure_string_added;
    // added another test string to test upgrades2
    string test_structure_string_added2;
    // test int
    int test_structure_int;
    // test bool
    bool test_structure_bool;
    // test float
    float test_structure_float;
    // test date
    date test_structure_date;
    // test time
    time test_structure_time;
    // test optional
    optional<string> test_structure_optional_string;
    // test map
    std::map<string, string> test_structure_map;
    // test vector
    std::vector<string> test_structure_vector;
    // test map
    std::map<string, string> test_structure_map2;
    // test vector
    std::vector<string> test_structure_vector2;
};

// Test enum
api(enum)
enum class upgrade_test_enum
{
    // EMPTY
    EMPTY,
    // SINGLE_VALUE
    SINGLE_VALUE,
    // TWO_VALUES
    TWO_VALUES
};

// upgrade_test_union_struct_one
api(struct)
struct upgrade_test_union_struct_one
{
    // test_string
    string test_string;
};

// upgrade_test_union_struct_two
api(struct)
struct upgrade_test_union_struct_two
{
    // test_int
    int test_int;
    // test string
    string test_string;
    // test string two
    string test_string_two;
};

// upgrade_test_union_struct_two
api(struct)
struct upgrade_test_union_struct_three
{
    // test_int
    bool test_bool;
};

// Test union
api(union)
union upgrade_test_union
{
    // Union string
    upgrade_test_union_struct_one one;
    // Union int
    upgrade_test_union_struct_two two;
};

api(struct)
struct upgrade_test_base
{
    // test string
    string test_string;
    // test int
    int test_int;
    // test bool
    bool test_bool;
    // test float
    float test_float;
    // test structure
    upgrade_test_structure test_structure;
};

api(struct)
struct upgrade_test_base_enum_child
{
    // test string
    string test_string;
    // test enum
    upgrade_test_enum test_enum;
};

api(struct)
struct upgrade_test_base_union_child
{
    // test string
    string test_string;
    // test union
    upgrade_test_union test_union;
};


api(struct)
struct test_1
{
    // test string
    string test_string;
    // test int
    int test_int;
    // test string
    string test_string_added;
    // test_int_added
    int test_int_added;
};

api(struct)
struct test_1a
{
    // test string
    string test_string;
    // test int
    int test_int;
    // test string
    string test_string_added;
};

api(struct)
struct test_2
{
    // test string
    string test_string;
    // test int
    int test_int;
    // test string optional
    optional<string> test_string_added_optional;
};

api(struct)
struct test_3
{
    // test string
    string test_string;
    // test int
    int test_int;
    // test string omissible
    omissible<string> test_string_added_omissible;
};

api(struct)
struct test_4
{
    // test string
    string test_string;
    // test int
    int test_int;
    // test array
    array<int> test_array_int;
};

api(struct)
struct test_5
{
    // test string
    string test_string;
    // test int
    int test_int;
    // test vector
    std::vector<test_1> test_vector;
};

api(struct)
struct test_5a
{
    // test string
    string test_string;
    // test int
    int test_int;
    // test vector
    std::vector<test_1a> test_vector;
};

api(struct)
struct test_6
{
    // test string
    string test_string;
    // test int
    int test_int;
    // test map2
    std::map<string, test_1> test_map;
};

api(struct)
struct test_7
{
    // test string
    string test_string;
    // test int
    int test_int;
    // test object_reference
    object_reference<test_1> test_object_reference;
};


api(struct)
struct test_8
{
    // test string
    string test_string;
    // test int
    int test_int;
    // added_union
    upgrade_test_union added_union;
};

api(struct)
struct test_9
{
    // test string
    string test_string;
    // test int
    int test_int;
    // nested struct
    test_1 nested_struct;
};

api(struct)
struct test_16
{
    // test string
    string test_string;
    // test int
    int test_int;
    // Test adding enum
    upgrade_test_enum added_enum;
};

api(enum)
enum class test_10
{
    // EMPTY
    EMPTY,
    // SINGLE_VALUE
    SINGLE_VALUE,
    // TWO_VALUES
    TWO_VALUES,
    // test
    TEST
};

api(enum)
enum class test_11
{
    // EMPTY
    EMPTY,
    // SINGLE_VALUE
    SINGLE_VALUE,
    // TWO_VALUES
    TWO_VALUES,
    // THREE_VALUES
    THREE_VALUES,
    // FOUR_VALUES
    FOUR_VALUES
};

api(enum)
enum class test_12
{
    // EMPTY
    EMPTY
};

api(union)
union test_13
{
    // Union string
    upgrade_test_union_struct_one one;
    // Union int
    upgrade_test_union_struct_two two;
    // union bool
    upgrade_test_union_struct_three three;
};

api(union)
union test_14
{
    // Union string
    upgrade_test_union_struct_one one;
};

api(union)
union test_15
{
    // Union string
    upgrade_test_union_struct_one one;
    // Union int
    upgrade_test_union_struct_two two;
};
////
////api(fun)
////string fun_test_1(test_1 t);
////
////api(fun)
////optional<string> fun_test_2(test_2 t);
////
////api(fun)
////string fun_test_3(test_3 t);
////
////api(fun)
////array<int> fun_test_4(test_4 t);
////
////api(fun)
////std::vector<int> fun_test_5(test_5 t);
////
////api(fun)
////std::map<string, int> fun_test_6(test_6 t);
////
////api(fun)
////object_reference<test_1> fun_test_7(test_7 t);
////
////api(fun)
////upgrade_test_union fun_test_8(test_8 t);
////
////api(fun)
////test_1 fun_test_9(test_9 t);
////
////api(fun)
////string fun_test_16(test_16 t);
////
////api(fun)
////string fun_test_10(test_10 t);
////
////api(fun)
////string fun_test_11(test_11 t);
////
////api(fun)
////string fun_test_12(test_12 t);
////
////api(fun)
////string fun_test_13(test_13 t);
////
////api(fun)
////string fun_test_14(test_14 t);
////
////api(fun)
////string fun_test_15(test_15 t);

}

#endif