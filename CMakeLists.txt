# Minimum version for CMake
cmake_minimum_required (VERSION 2.6)

# Project's name
project (kd-tree)

# configure a header file to pass some of the CMake settings
# to the source code
# configure_file (
#   "${PROJECT_BINARY_DIR}/kd_tree.h"
#   )

# add the binary tree to the search path for include files
# so that we will find kd_tree.h
#include_directories("src")

#set(SOURCES src)

# build the executable
add_executable(test src/nearest_neighbor.cpp src/kd_tree.h)
