#!/bin/bash

# Exit if any build steps fail.
set -e

# if [ $# -lt 1 ]; then
#     echo "usage: build.sh <root-astroid-directory> <make-arguments>"
#     exit 1
# fi

# Capture the directory where build.sh is stored.
# source_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# if [ ! -d "$1/build" ]; then
#   mkdir -p $1/build
# fi
cd ~/scratch/astroid/build

if [ ! -d "preprocessor" ]; then
  mkdir preprocessor
  cd preprocessor
  # cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=release -DASTROID_LIB_DIR=$1/lib "$source_dir/preprocessor"
  cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=release -DASTROID_LIB_DIR=~/scratch/astroid/lib ~/scratch/astroid/src/cradle/preprocessor
  make -j4 install
  cd ..
fi



# if [ ! -d "cradle-release" ]; then
#   mkdir cradle-release
# fi

cd cradle-release
# cmake -G"Unix Makefiles"  -DCMAKE_BUILD_TYPE=release -DASTROID_LIB_DIR=$1/lib $source_dir
cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=release -DASTROID_LIB_DIR=~/scratch/astroid/lib ~/scratch/astroid/src/cradle
make -j4 cradle
make -j4 install
cd ..

# mkdir cradle-debug
# cd cradle-debug
# cmake -G"Unix Makefiles" -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=debug -DASTROID_LIB_DIR=$1/lib $source_dir
# make $2 cradle
# make $2 install
# cd ..
