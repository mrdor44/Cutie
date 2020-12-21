## General Settings
cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 17)
include(CTest)
include(FetchContent)

## Verifications
function(verify_variable variable_name)
    if (NOT DEFINED ${variable_name})
        message(FATAL_ERROR "Variable ${variable_name} must be defined")
    endif ()
endfunction()

verify_variable(CUTIE_DIR)
get_property(languages GLOBAL PROPERTY ENABLED_LANGUAGES)
if (NOT "CXX" IN_LIST languages)
    message(FATAL_ERROR "Project must be defined with language CXX")
endif ()

## Dependencies
# GTest
FetchContent_Declare(googletest
    GIT_REPOSITORY
        https://github.com/google/googletest.git
    GIT_TAG
        release-1.8.1 # TODO update to 1.10?
)
set(INSTALL_GTEST OFF)
FetchContent_GetProperties(googletest)
FetchContent_MakeAvailable(googletest)
set(GOOGLETEST_DIR ${googletest_SOURCE_DIR})

# Subhook
FetchContent_Declare(subhook
    GIT_REPOSITORY
        https://github.com/Zeex/subhook.git
    GIT_TAG
        v0.7 # TODO update to v0.8.1?
)
set(SUBHOOK_STATIC ON)
set(SUBHOOK_TESTS OFF)
FetchContent_MakeAvailable(subhook)
set(SUBHOOK_DIR ${subhook_SOURCE_DIR})

# C-Mock
FetchContent_Declare(c_mock
    GIT_REPOSITORY
        https://github.com/hjagodzinski/C-Mock.git
    GIT_TAG
        v0.2.1
)
FetchContent_GetProperties(c_mock)
FetchContent_MakeAvailable(c_mock)
set(C_MOCK_DIR ${c_mock_SOURCE_DIR})

## Targets
set(TEST_TARGETS)

## Functions

# Define a new target to run a single test file.
#
# Usage:
#   add_cutie_test_target(TEST test [SOURCES sources...] [COMPILER_FLAGS compile_flags...] [COMPILER_DEFINITIONS compile_defs...] [LINKER_FLAGS link_flags...])
#   TEST followed by the test source file
#   SOURCES followed by additional (optional) source files list required for the test
#   COMPILER_FLAGS followed by a list of compile time flags
#   COMPILER_DEFINITIONS followed by a list of definitions for the compiler
#   LINKER_FLAGS followed by a list of link time flags
#
#   TODO improve example
# Example:
#     add_cutie_test_target(TEST test/a.cpp SOURCES src/a.c src/b.c)
function(add_cutie_test_target)
    # parse arguments
    cmake_parse_arguments(PARSE_ARGV 0 TEST "" "TEST" "SOURCES;COMPILER_FLAGS;COMPILER_DEFINITIONS;LINKER_FLAGS")
    get_filename_component(TEST_NAME ${TEST_TEST} NAME_WE)

    # define test target
    add_executable(${TEST_NAME} ${TEST_TEST} ${TEST_SOURCES})

    # define flags
    set(COVERAGE_FLAGS -fprofile-arcs -ftest-coverage --coverage)
    set(C_MOCK_LINKER_FLAGS "-rdynamic -Wl,--no-as-needed -ldl")

    target_include_directories(${TEST_NAME}
        PUBLIC
            ${CUTIE_DIR}
            ${GOOGLETEST_DIR}/googlemock/include
            ${GOOGLETEST_DIR}/googletest/include
            ${CMOCK_DIR}/include
            ${SUBHOOK_DIR}
    )
    
    # set build options
    target_compile_options(${TEST_NAME}
        PRIVATE
            ${COMPILER_FLAGS}
            ${COVERAGE_FLAGS}
    )
    target_compile_definitions(${TEST_NAME}
        PRIVATE
            ${COMPILER_DEFINITIONS}
    )
    target_link_libraries(${TEST_NAME}
        PUBLIC
            gmock_main
            subhook
    )
    target_link_options(${TEST_NAME}
        PRIVATE
            ${C_MOCK_LINKER_FLAGS}
            ${COVERAGE_FLAGS}
            ${LINKER_FLAGS}
    )

    set(TEST_TARGETS ${TEST_TARGETS} ${TEST_NAME} PARENT_SCOPE)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Define the `all_tests` target that runs all tests added with add_cutie_test_target()
# Function has no parameters
function(add_cutie_all_tests_target)
    add_custom_target(all_tests
            COMMAND ctest
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            VERBATIM)
    add_dependencies(all_tests ${TEST_TARGETS})
endfunction()

# Define the following two targets:
#   1. `coverage` runs all tests and collects coverage
#   2. `clean_coverage` cleans coverage information
# The collected coverage report resides in the coverage/ directory under the project's directory.
# Function has no parameters
function(add_cutie_coverage_targets)
    include(${CUTIE_DIR}/inc/CodeCoverage.cmake)
    set(COVERAGE_DIR coverage)
    setup_target_for_coverage_lcov(
            NAME ${COVERAGE_DIR}
            EXECUTABLE ctest
            EXCLUDE "${CUTIE_DIR}/*" "/usr/include/*")
    add_custom_target(clean_coverage
            rm --recursive --force ${COVERAGE_DIR}
            COMMAND find -iname "*.gcda" -delete
            COMMAND find -iname "*.gcno" -delete
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            VERBATIM
            COMMENT "Deleting coverage information. Rebuild after this.")
endfunction()
