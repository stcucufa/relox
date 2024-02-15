#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hamt.h"
#include "value.h"

void test_numbers(void) {
    HAMT hamt;
    hamt_init(&hamt);

    for (size_t i = 0; i < 1131152; ++i) {
        hamt_set(&hamt, VALUE_FROM_NUMBER(i), VALUE_FROM_NUMBER(0));
    }
    // hamt_free(&hamt);
}

int main(int argc, char* argv[argc + 1]) {
    test_numbers();
    return EXIT_SUCCESS;
}
