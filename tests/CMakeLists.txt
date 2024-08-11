##--------------------------------------------------------------
## Copyright (c) David Bosnich <david.bosnich.public@gmail.com>
##
## This code is licensed under the MIT License, a copy of which
## can be found in the license.txt file included at the root of
## this distribution, or at https://opensource.org/licenses/MIT
##--------------------------------------------------------------

# Early out if generating a sub project.
if (NOT CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    return()
endif()

# Gather test files.
file(GLOB_RECURSE test_files *.h *.cpp)

# Group test files for the IDE.
source_group(TREE "${PROJECT_SOURCE_DIR}/tests"
             PREFIX "tests"
             FILES ${test_files})

# Group header files for the IDE.
source_group(TREE "${PROJECT_SOURCE_DIR}/include"
             PREFIX "include"
             FILES ${header_files})

# Fetch the Catch2 dependency.
Include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v2.13.10 # This is the last header only version
)
FetchContent_MakeAvailable(Catch2)

# Define the test executable.
set(TEST_TARGET "${PROJECT_NAME}_tests")
add_executable(${TEST_TARGET} ${test_files})
target_link_libraries(${TEST_TARGET} ${LIB_TARGET} Catch2::Catch2)
target_include_directories(${TEST_TARGET} PRIVATE .)
target_compile_options(${TEST_TARGET} PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:
    $<$<CXX_COMPILER_ID:MSVC>: /GR- /W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fno-rtti -Wall -Werror -Wextra>
  >
)

# Add the test executable to the RUN_TESTS target.
add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})
