#include <math.h>
#include <string.h>

#include "object.h"

#ifdef DEBUG
#include <stdio.h>
#endif

String* string_copy(const char* start, size_t length) {
    String* string = malloc(sizeof(String));
    string->length = length;
    string->chars = malloc(length + 1);
    memcpy(string->chars, start, length);
    string->chars[length] = 0;
#ifdef DEBUG
    fprintf(stderr, "+++ string_copy() new string \"%s\" (%p, %zu chars).\n",
        string->chars, (void*)string->chars, string->length);
#endif
    return string;
}

String* string_concatenate(String* x, String* y) {
    String* string = malloc(sizeof(String));
    string->length = x->length + y->length;
    string->chars = malloc(string->length + 1);
#ifdef DEBUG
    fprintf(stderr, "+++ string_concatenate() new string \"%s\" (%p, %zu chars).\n",
        string->chars, (void*)string->chars, string->length);
#endif
    memcpy(string->chars, x->chars, x->length);
    memcpy(string->chars + x->length, y->chars, y->length);
    string->chars[string->length] = 0;
    return string;
}

String* string_exponent(String* x, double n) {
    size_t m = (size_t)round(n > 0 ? n : 0);
    String* string = malloc(sizeof(String));
    string->length = x->length * m;
    string->chars = malloc(string->length + 1);
#ifdef DEBUG
    fprintf(stderr, "+++ string_exponent() new string \"%s\" (%p, %zu chars).\n",
        string->chars, (void*)string->chars, string->length);
#endif
    for (size_t i = 0; i < m; ++i) {
        memcpy(string->chars + i * x->length, x->chars, x->length);
    }
    string->chars[string->length] = 0;
    return string;
}

void string_free(String* string) {
#ifdef DEBUG
    fprintf(stderr, "--- string_free() \"%s\" (%p, %zu chars).\n",
        string->chars, (void*)string->chars, string->length);
#endif
    free(string->chars);
    free(string);
}
