#include <math.h>
#include <string.h>

#include "object.h"

#ifdef DEBUG
#include <stdio.h>
#endif

static String* string_new(size_t length) {
    String* string = malloc(sizeof(String) + length + 1);
    string->length = length;
#ifdef DEBUG
    fprintf(stderr, "+++ string_new() (%p, %zu chars).", (void*)string->chars, string->length);
#endif
    return string;
}

String* string_copy(const char* start, size_t length) {
    String* string = string_new(length);
    memcpy(string->chars, start, length);
    string->chars[length] = 0;
    return string;
}

String* string_concatenate(String* x, String* y) {
    String* string = string_new(x->length + y->length);
    memcpy(string->chars, x->chars, x->length);
    memcpy(string->chars + x->length, y->chars, y->length);
    string->chars[string->length] = 0;
    return string;
}

String* string_exponent(String* x, double n) {
    size_t m = (size_t)round(n > 0 ? n : 0);
    String* string = string_new(x->length * m);
    for (size_t i = 0; i < m; ++i) {
        memcpy(string->chars + i * x->length, x->chars, x->length);
    }
    string->chars[string->length] = 0;
    return string;
}

String* string_from_number(double n) {
    String* string = string_new((size_t)snprintf(NULL, 0, "%g", n));
    snprintf(string->chars, string->length + 1, "%g", n);
    return string;
}
