# Cutie: The complete C++ UT framework

**Cutie** is a *complete* C/C++ unit testing framework. It is said to be complete as it integrates several existing C++ UT frameworks and libraries into a single framework that provides all the needed functionality for your C++ testing purposes.

## Why was Cutie created?

Cutie was created after painfully trying to test C code with stubs and mocks. GoogleTest provides a great UT framework, but it's mock counterpart, GoogleMock, doesn't support mocking pure C functions (it only supports C++ methods). Using Subhook and CMock, Cutie enables setting GoogleMock mocks on C functions, harnessing its expressiveness to mock C functions.

## What's in the box?

As said before, Cutie integrates several UT frameworks and libraries and provides additional functionality on top of them.   We will now survey the features provided within Cutie.

### Full Unit Testing Framework

Cutie employs **GoogleTest** to provide a full unit testing framework, including test definitions, a built in runner, assertions, etc. For the full specification of what's provided in GoogleTest, refer to the full [GoogleTest Documentation](https://github.com/google/googletest/blob/master/README.md).

Some of the interesting documentation pages are:

- [Primer](https://github.com/google/googletest/blob/master/googletest/docs/primer.md): Details basic concepts, assertions and writing simple tests.
- [Advanced](https://github.com/google/googletest/blob/master/googletest/docs/advanced.md): Explicit Success and Failure, Predicate Assertions for Better Error Messages, Floating Point Comparison, Death Tests, Using Assertions in Sub-Routines, Propagating Fatal Failures, Logging Additional Information, Sharing Resources Between Tests in the Same Test Case, Global Set-Up and Tear-Down, Value Parameterized Tests, Testing Private Code, Getting the Current Test's Name, Running Test Programs: Advanced Options.
- [FAQ](https://github.com/google/googletest/blob/master/googletest/docs/faq.md): Contains many interesting entries.

### Hooks

**Subhook** is a library for setting and managing hooks on C functions. Cutie employs Subhook to stub C functions, and later, using CMock, mocking them using GoogleMock.

To set hooks using Subhook alone, you can check out the [Subhook Documentation](https://github.com/Zeex/subhook/blob/master/README.md). However, we recommend using Cutie's `hook.hpp` header file to set hooks. The documentation is within [`hook.hpp`](hook.hpp) itself.

### Mocks

Cutie's main feature, and the reason why it was created, was to support **GoogleMock** mocks on C functions. Cutie implements this using the CMock library.

To set mocks on C functions, refer to the [`mock.hpp`](mock.hpp), which contains both the documentation and the implementation. Cutie's mocks enable setting GoogleMock expectations on the mocks. For the full capabilities of GoogleMock, refer to the [GoogleMock Documentation](https://github.com/google/googletest/blob/master/googlemock/README.md) and [GoogleMock For Dummies](https://github.com/google/googletest/blob/master/googlemock/docs/for_dummies.md).

## GoogleMock or GoogleTest?

So what's the difference between GoogleMock and GoogleTest?

**GoogleTest** was created as a framework for writing unit tests, including test methods, auto-generated main function, set-up and tear-down functions and such. **GoogleMock** was created as a follow-up framework for creating automatic mock classes, for testing C++ code.

Afterwards, the developers at Google decided that the two frameworks should be united, so they united them both into the GoogleTest project, that now contains both GoogleTest and GoogleMock. As GoogleMock relies on GoogleTest, including GoogleMock's headers will also include GoogleTest's headers.

# Getting Started with Cutie

For a sample repository that uses Cutie for unit tests, refer to [datastructures-algorithms](https://github.com/mrdor44/datastructures-algorithms).

## Include Cutie in your project

Cutie provides a [Cutie.cmake](Cutie.cmake) CMake file that should be included from your own project's CMakeList.txt file. An example project's CMakeLists.txt file (fill in <RELEVANT_TAG>):

```cmake
cmake_minimum_required(VERSION 3.10)
project(my_project CXX C)

add_executable(my_project src/module1.c src/module2.c src/main.c)

FetchContent_Declare(cutie
    GIT_REPOSITORY
        https://github.com/dolevelbaz/Cutie.git
    GIT_TAG
        <RELEVANT_TAG>
)
FetchContent_GetProperties(cutie)
if (NOT cutie_POPULATED)
    FetchContent_Pupulate(cutie)
    # without add_subdirectories since Cutie doesn't support it
endif()

set(CUTIE_DIR ${cutie_SOURCE_DIR})
include(${CUTIE_DIR}/Cutie.cmake)

add_cutie_test_target(TEST test/test_module1.cpp SOURCES src/module1.c)
add_cutie_test_target(TEST test/test_module2.cpp SOURCES src/module2.c)
add_cutie_all_tests_target()
add_cutie_coverage_targets()
```

As you can see from the above example, including Cutie in your project is fairly easy. All you need to do is:

1. Set your project's languages to `CXX` and `C`.
2. Add Cutie as a CMake dependency.
3. Set the variable `CUTIE_DIR` to the relative or absolute path to Cutie's directory.
4. Call `add_cutie_test_target` for each test you've written, passing it the test file and all related source files.
   **Note:** Don't pass a source file that has a `main()` function to `add_cutie_test_target`, as it provides its own `main()` function, using GoogleTest.
5. Call `add_cutie_all_tests_target` if you want to have the `all_tests` target.
6. Call `add_cutie_coverage_targets` if you want to have the `coverage` and `clean_coverage` targets.

## Write your first test

After including Cutie, you can write your first test. A sample test file (can also be found in [samples/sample_test.cpp](samples/sample_test.cpp)):

```c++
// This is a sample test
// Delete it when you have real tests

extern "C" {
#include <stdio.h>
#include <stdarg.h>
}

#include "mock.hpp"
#include "hook.hpp"

using namespace testing;

//@formatter:off
DECLARE_HOOKABLE(sprintf); // Sadly, GMock doesn't support mocking ellipsis...
DECLARE_MOCKABLE(fclose, 1);
DECLARE_MOCKABLE(fwrite, 4);
DECLARE_MOCKABLE(fopen, 2);
//@formatter:on

int tested_function(const char* file);

int __STUB__sprintf(char* str, const char* format, ...) {
    SCOPE_REMOVE_HOOK(sprintf);
    std::cout << "I'm in stub!" << std::endl;
    sprintf(str, "%s/%s", "foo", "bar");
    return 10;
}

TEST(Sample, Test) {
    INSTALL_HOOK(sprintf, __STUB__sprintf);
    INSTALL_MOCK(fclose);

    INSTALL_EXPECT_CALL(fopen, _, _).WillRepeatedly(Return(nullptr));
    INSTALL_EXPECT_CALL(fwrite, _, _, _, _).WillOnce(Return(0)).WillRepeatedly(Return(0));

    InSequence s;
    CUTIE_ON_CALL(fclose, _).WillByDefault(Return(10));
    for (int i = 0; i < 4; ++i) {
        CUTIE_EXPECT_CALL(fclose, _).WillOnce(Return(i));
    }

    EXPECT_EQ(10, tested_function("dummy_file"));
    EXPECT_EQ(11, tested_function("dummy_file"));
    EXPECT_EQ(12, tested_function("dummy_file"));
    EXPECT_EQ(13, tested_function("dummy_file"));
}
```

## Run your tests

After writing your test, you can build and run it using the `sample_test` CMake target. The target will use GoogleTests's test runner to run your test.

If you've used `add_cutie_all_tests_target`, you can also run the `all_tests` target.

## Analyze Code Coverage

Cutie provides two more CMake targets: `coverage` and `clean_coverage`:

* The `clean_coverage` target is pretty straightforward, as it simply cleans up any coverage information already collected.
* The `coverage` target reruns your tests while collecting coverage information.

After using `coverage` to rerun tests and collect coverage information, the coverage information is saved in \<cmake-build-directory\>/coverage. For example, if your CMake build directory is `cmake-build-debug`, the coverage information is saved in `cmake-build-debug/coverage`. To view the coverage information, open the `index.html` file in the coverage directory using your favorite web browser.