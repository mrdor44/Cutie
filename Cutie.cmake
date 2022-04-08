# Cutie
# ~~~~~
# Cutie is a C++ UT framework, based on GoogleTest, GoogleMock, SubHook and CMock libraries.
# Cutie provides the following facilities:
#   1. A framework for writing unit tests and assertions (using GoogleTest)
#   2. A framework for setting hooks on functions for testing purposes (using hook.hpp)
#   3. A framework for setting mocks on functions for testing purposes (using mock.hpp)
# Specific documentation can be found in the header files and the READMEs in GoogleTest and GoogleMock.
#
# Using Cutie in your CMakeLists.txt
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Using Cutie in your Cutie.cmake boils down to including Cutie's Cutie.cmake.
# Follow these steps:
#   1. Set your project's languages to CXX and C
#   2. Set the CUTIE_DIR variable to Cutie's directory within your project (will probably be "Cutie"). For example:
#          set(CUTIE_DIR Cutie)
#   3. Include Cutie's Cutie.cmake. For example:
#          include(${CUTIE_DIR}/Cutie.cmake)
#   4. Call Cutie's add_cutie_test_target for each test you have. For example:
#          add_cutie_test_target(TEST test/a.cpp SOURCES src/a.c src/b.c)
#   5. Call Cutie's add_cutie_all_tests_target to add the `all_tests` target. This is optional.
#   6. Call Cutie's add_cutie_coverage_targets to add the `coverage` and `clean_coverage` targets. This is optional.
#
# Running Tests
# ~~~~~~~~~~~~~
# After integrating Cutie, run all tests using the `tests` target.
#
# Collecting Coverage
# ~~~~~~~~~~~~~~~~~~~
# After integrating Cutie, run all tests and collect coverage using the `coverage` target.
# The coverage will be collected to ${PROJECT_BINARY_DIR}/coverage.
# The coverage will be written as a series of HTML pages for your convenience.
# To view the coverage report, open ${PROJECT_BINARY_DIR}/coverage/index.html in your favorite web browser.
#
# Cleaning Coverage
# ~~~~~~~~~~~~~~~~~
# To clean coverage data, use the `clean_coverage` target.
#
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
include(CTest)


if (WIN32)
  option(USE_DLFCN_WIN32_PACKAGE "Use dlfcn-win32 system package (if false dlfcn is build from source)" FALSE)
  if(${USE_DLFCN_WIN32_PACKAGE})
    #MSYS2 case (dlfcn package can be installed)
    find_package(dlfcn-win32 REQUIRED)
    set(CMAKE_DL_LIBS dlfcn-win32::dl)
    set(BUILD_DLFCN FALSE)
  else()
    set(CMAKE_DL_LIBS dl)
    set(BUILD_DLFCN TRUE)
  endif()
endif ()

## Functions
function(verify_variable variable_name)
    if (NOT DEFINED ${variable_name})
        message(FATAL_ERROR "Variable ${variable_name} must be defined")
    endif ()
endfunction()

## Verifications
verify_variable(CUTIE_DIR)
get_property(languages GLOBAL PROPERTY ENABLED_LANGUAGES)
if (NOT "CXX" IN_LIST languages)
    message(FATAL_ERROR "Project must be defined with language CXX")
endif ()

## Global Variables
set(TEST_TARGETS)

# Defines a new target to run a single test file
# Usage:
#     add_cutie_test_target(TEST test [SOURCES sources...])
#     'test' is the test file that should be executed
#     'sources' is an optional list of source files that are required for the test
#
# Example:
#     add_cutie_test_target(TEST test/a.cpp SOURCES src/a.c src/b.c)
function(add_cutie_test_target)
    ## Dependencies directories
    set(GOOGLETEST_DIR ${CUTIE_DIR}/googletest)
    set(SUBHOOK_DIR ${CUTIE_DIR}/subhook)
    set(CMOCK_DIR ${CUTIE_DIR}/C-Mock)
    if(${BUILD_DLFCN})
	if (NOT DEFINED DLFCN_DIR)
	message (FATAL_ERROR "DLFCN build required, please provide dlfcn location using variable DLFCN_DIR")
	endif()
    endif()
    set(GOOGLETEST_BIN_DIR ${GOOGLETEST_DIR}/build)
    set(SUBHOOK_BIN_DIR ${SUBHOOK_DIR}/build)
    if(${BUILD_DLFCN})
    set(DLFCN_BIN_DIR ${DLFCN_DIR}/build)
    endif()

    ## Compiler & Linker flags
    set(COVERAGE_FLAGS -fprofile-arcs -ftest-coverage --coverage)
    
    if(WIN32)
    set(CMOCK_LINKER_FLAGS "-Wl,--export-all-symbols,--no-as-needed -O0")
    else()
    set(CMOCK_LINKER_FLAGS "-rdynamic -Wl,--no-as-needed -ldl")
    endif()

    ## Compiling dependencies
    if (NOT DEFINED _CUTIE_DEPENDENCIES_COMPILED)
        set(INSTALL_GTEST OFF)
        add_subdirectory(${GOOGLETEST_DIR} ${GOOGLETEST_BIN_DIR} EXCLUDE_FROM_ALL)
        set(SUBHOOK_STATIC ON)
        set(SUBHOOK_TESTS OFF)
        add_subdirectory(${SUBHOOK_DIR} ${SUBHOOK_BIN_DIR} EXCLUDE_FROM_ALL)
        if(${BUILD_DLFCN})
        add_subdirectory(${DLFCN_DIR} ${DLFCN_BIN_DIR} EXCLUDE_FROM_ALL)
        endif()
        set(_CUTIE_DEPENDENCIES_COMPILED 1 PARENT_SCOPE)
    endif ()

    ## Define the test target
    cmake_parse_arguments(PARSE_ARGV 0 TEST "" TEST SOURCES)
    get_filename_component(TEST_NAME ${TEST_TEST} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_TEST} ${TEST_SOURCES})
    target_include_directories(${TEST_NAME} PUBLIC
            ${CUTIE_DIR}
            ${GOOGLETEST_DIR}/googlemock/include
            ${GOOGLETEST_DIR}/googletest/include
            ${CMOCK_DIR}/include
            ${SUBHOOK_DIR}
            "$<$<BOOL:${BUILD_DLFCN}>:${DLFCN_DIR}/src>"
            )
    target_compile_options(${TEST_NAME} PUBLIC ${COVERAGE_FLAGS})
    target_link_libraries(${TEST_NAME} ${CMAKE_DL_LIBS} gmock_main subhook ${CMOCK_LINKER_FLAGS} ${COVERAGE_FLAGS})
    set(TEST_TARGETS ${TEST_TARGETS} ${TEST_NAME} PARENT_SCOPE)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Defines the `all_tests` target that runs all tests added with add_cutie_test_target()
# Function has no parameters
function(add_cutie_all_tests_target)
    add_custom_target(all_tests
            COMMAND ctest
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            VERBATIM)
    add_dependencies(all_tests ${TEST_TARGETS})
endfunction()

# Defines the following two targets:
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
