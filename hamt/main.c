#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../hamt.h"
#include "../value.h"

int main(int argc, char* argv[argc + 1]) {
    HAMT hamt;
    hamt_init(&hamt);

    hamt_set(&hamt, VALUE_FROM_NUMBER(29), VALUE_FROM_NUMBER(0));
#ifdef DEBUG
    hamt_debug(&hamt);
#endif

    hamt_set(&hamt, VALUE_FROM_NUMBER(10), VALUE_FROM_NUMBER(1));
#ifdef DEBUG
    hamt_debug(&hamt);
#endif

    hamt_set(&hamt, VALUE_FROM_NUMBER(26), VALUE_FROM_NUMBER(31));
#ifdef DEBUG
    hamt_debug(&hamt);
#endif

    hamt_set(&hamt, VALUE_FROM_NUMBER(33), VALUE_FROM_NUMBER(5));
#ifdef DEBUG
    hamt_debug(&hamt);
#endif

    hamt_set(&hamt, VALUE_FROM_NUMBER(32), VALUE_FROM_NUMBER(5));
#ifdef DEBUG
    hamt_debug(&hamt);
#endif

    hamt_free(&hamt);
    return EXIT_SUCCESS;
}
