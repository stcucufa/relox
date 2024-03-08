#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../hamt.h"
#include "../value.h"

int main(int argc, char* argv[argc + 1]) {
    HAMT hamt;
    hamt_init(&hamt);

    HAMT* h1 = hamt_with(&hamt, VALUE_FROM_NUMBER(31), VALUE_FROM_NUMBER(41));
    hamt_set(&hamt, VALUE_FROM_NUMBER(17), VALUE_FROM_NUMBER(23));
    hamt_set(&hamt, VALUE_FROM_NUMBER(31), VALUE_FROM_NUMBER(51));
    HAMT* h2 = hamt_with(&hamt, VALUE_FROM_NUMBER(111), VALUE_FROM_NUMBER(223));
    h2 = hamt_with(h2, VALUE_FROM_NUMBER(31), VALUE_FROM_NUMBER(71));
    fprintf(stderr, "hamt <%p> ", (void*)&hamt);
    hamt_debug(&hamt);
    fprintf(stderr, "h1   <%p> ", (void*)h1);
    hamt_debug(h1);
    fprintf(stderr, "h2   <%p> ", (void*)h2);
    hamt_debug(h2);

    hamt_free(h1);
    hamt_free(h2);
    fprintf(stderr, "hamt <%p> ", (void*)&hamt);
    hamt_debug(&hamt);

    // TODO test numbers from 0 to 131151 (double collision)
    for (size_t i = 0; i < 1131152; ++i) {
        hamt_set(&hamt, VALUE_FROM_NUMBER(i), VALUE_FROM_NUMBER(0));
    }
    hamt_free(&hamt);
    return EXIT_SUCCESS;
}
