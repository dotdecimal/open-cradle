cmake_minimum_required(VERSION 2.6)
project(cradle_tests)

include(../cmake/UseCradle.cmake)

set(CRADLE_INCLUDE_UNIT_TESTING ON)
set(CRADLE_INCLUDE_GUI OFF)
set(CRADLE_INCLUDE_PARETO_NAVIGATION ON)
set(CRADLE_INCLUDE_DATABASE ON)
set(CRADLE_INCLUDE_STANDARD_IMAGE_IO ON)
set(CRADLE_INCLUDE_APP_CONFIG ON)
set(CRADLE_INCLUDE_DICOM_IO ON)
set(CRADLE_INCLUDE_SCRIPTING ON)
add_cradle(cradle "${CMAKE_CURRENT_SOURCE_DIR}/..")

enable_testing()

file (GLOB_RECURSE test_files RELATIVE ${CMAKE_SOURCE_DIR} "*.cpp")

foreach(test_file ${test_files})
    get_filename_component(path ${test_file} PATH)
    get_filename_component(basename ${test_file} NAME_WE)
    string(REPLACE "/" "__" test_name "${path}/${basename}")
    add_executable(${test_name} ${test_file})
    use_cradle(${test_name} cradle)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
