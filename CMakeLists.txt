##--------------------------------------------------------------
## Copyright (c) David Bosnich <david.bosnich.public@gmail.com>
##
## This code is licensed under the MIT License, a copy of which
## can be found in the license.txt file included at the root of
## this distribution, or at https://opensource.org/licenses/MIT
##--------------------------------------------------------------

# Specify minimum tested CMake version.
cmake_minimum_required(VERSION 3.28)

# Define project settings.
project(simple_application
        LANGUAGES CXX
        VERSION 1.0)

# Gather header files.
file(GLOB_RECURSE header_files include/*.h)

# Group header files for the IDE.
source_group(TREE "${PROJECT_SOURCE_DIR}/include"
             PREFIX "include"
             FILES ${header_files})

# Define the library (header only).
set(LIB_TARGET "${PROJECT_NAME}")
add_library(${LIB_TARGET} INTERFACE)
target_sources(${LIB_TARGET} INTERFACE ${header_files})
target_compile_features(${LIB_TARGET} INTERFACE cxx_std_11)
target_include_directories(${LIB_TARGET} INTERFACE include)

# Customize the predefined targets folder name.
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "HelperTargets")

# Add documentation.
add_subdirectory("docs")

# Add tests.
enable_testing()
add_subdirectory("tests")
