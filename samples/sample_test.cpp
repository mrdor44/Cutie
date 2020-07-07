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