#!/bin/bash

# change to the build directory
mkdir -p ./build
cd ./build

# generate the build files
cmake .. -DCMAKE_TOOLCHAIN_FILE=../../vcpkg/scripts/buildsystems/vcpkg.cmake \
	-DENABLE_TESTS=On -DENABLE_LUA=On
#cmake .. -DENABLE_TESTS=On -DENABLE_LUA=On

# run build
make clean all

# go back to parent directory
cd ..
