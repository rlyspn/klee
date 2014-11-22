#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t my_strlen(const char *s) {
    size_t len = 0;
    while(*s++) len ++;

    return len;
}

int main(int argc, char *argv[]) {
    // Only returns 0 when one argument is passed, and that argument
    // begins with 'a' and is three characters long.
    if(argc == 2 && my_strlen(argv[1]) == 3 && argv[1][0] == 'a') {
        return 0;
    }

    exit(1);
}
