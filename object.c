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
    return string;
}

void string_free(String* string) {
#ifdef DEBUG
    fprintf(stderr, "--- string_free() \"%s\" (%p, %zu chars).\n",
        string->chars, (void*)string->chars, string->length);
#endif
    free(string->chars);
}
