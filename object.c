#include <math.h>
#include <stdio.h>
#include <string.h>

#include "object.h"

// Cf. https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
uint64_t bytes_hash(char* bytes, size_t length) {
    uint64_t hash = 0xcbf29ce484222325;
    for (size_t i = 0; i < length; ++i) {
        hash ^= (uint8_t)bytes[i];
        hash *= 0x100000001b3;
    }
    return hash;
}

static String* string_new(size_t length) {
    String* string = malloc(sizeof(String) + length + 1);
    string->length = length;
#ifdef DEBUG
    fprintf(stderr, "+++ string_new() (%p, length: %zu).", (void*)string->chars, string->length);
#endif
    return string;
}

String* string_copy(const char* start, size_t length) {
    String* string = string_new(length);
    memcpy(string->chars, start, length);
    string->chars[length] = 0;
    string->hash = bytes_hash(string->chars, string->length);
    return string;
}

String* string_concatenate(String* x, String* y) {
    String* string = string_new(x->length + y->length);
    memcpy(string->chars, x->chars, x->length);
    memcpy(string->chars + x->length, y->chars, y->length);
    string->chars[string->length] = 0;
    string->hash = bytes_hash(string->chars, string->length);
    return string;
}

String* string_exponent(String* x, double n) {
    String* string = string_new(x->length * n);
    for (size_t i = 0; i < n; ++i) {
        memcpy(string->chars + i * x->length, x->chars, x->length);
    }
    string->chars[string->length] = 0;
    string->hash = bytes_hash(string->chars, string->length);
    return string;
}

String* string_from_number(double n) {
    String* string = string_new((size_t)snprintf(NULL, 0, "%g", n));
    snprintf(string->chars, string->length + 1, "%g", n);
    string->hash = bytes_hash(string->chars, string->length);
    return string;
}

bool string_equal(String* s, String* t) {
    return s->length == t->length && s->hash == t->hash && memcmp(s->chars, t->chars, s->length) == 0;
}
