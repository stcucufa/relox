#include <inttypes.h>
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
    hamt_free(&hamt);
}

double fibonacci(double i) {
    return i < 2 ? i : fibonacci(i - 1) + fibonacci(i - 2);
}

double fibonacci_memoized(double i, HAMT* hamt) {
    if (i < 2) {
        return i;
    }

    Value key = VALUE_FROM_NUMBER(i);
    Value v = hamt_get(hamt, key);
    if (!VALUE_IS_NONE(v)) {
        return v.as_double;
    }
    double f1 = fibonacci_memoized(i - 1, hamt);
    double f2 = fibonacci_memoized(i - 2, hamt);
    double f = f1 + f2;
    hamt_set(hamt, key, VALUE_FROM_NUMBER(f));
    return f;
}

uint64_t test_fibonacci(double i) {
    HAMT hamt;
    hamt_init(&hamt);

    double f = fibonacci_memoized(i, &hamt);
    hamt_free(&hamt);
    return f;
}

int main(int argc, char* argv[argc + 1]) {
    printf("%" PRId64 "\n", test_fibonacci(10000));
    return EXIT_SUCCESS;
}
