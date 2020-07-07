#include <stdio.h>

int tested_function(const char* file) {
    FILE* fd = fopen(file, "w");
    fwrite("abcd", 4, 1, fd);
    int ret = fclose(fd);
    char s[20] = {0};
    return ret + sprintf(s, "%s/%s", "foo", "bar");
}
