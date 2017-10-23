cmake_minimum_required(VERSION 2.6)
project(generators)

include("../cmake/UseCradle.cmake")

add_cradle(cradle "${CMAKE_CURRENT_SOURCE_DIR}/..")

find_package(SDL REQUIRED)

add_executable(isobands isobands.cpp)
use_cradle(isobands cradle)
