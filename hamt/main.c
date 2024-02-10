#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../hamt.h"
#include "../value.h"

int main(int argc, char* argv[argc + 1]) {
    HAMT hamt;
    hamt_init(&hamt);

    // TODO test numbers from 0 to 131151 (double collision)
    for (size_t i = 0; i < 1131152; ++i) {
        hamt_set(&hamt, VALUE_FROM_NUMBER(i), VALUE_FROM_NUMBER(0));
    }
    hamt_free(&hamt);
    return EXIT_SUCCESS;
}
