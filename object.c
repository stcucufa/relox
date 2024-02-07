#include <string.h>

#include "object.h"

static String* value_string_new(void) {
    String* string = malloc(sizeof(String));
    string->obj = malloc(sizeof(Obj));
    return string;
}

String* string_copy(const char* start, size_t length) {
    String* string = value_string_new();
    string->length = length;
    string->chars = malloc(length + 1);
    memcpy(string->chars, start, length);
    string->chars[length] = 0;
    return string;
}

String* string_concatenate(String* x, String* y) {
    String* string = value_string_new();
    string->length = x->length + y->length;
    string->chars = malloc(string->length + 1);
    memcpy(string->chars, x->chars, x->length);
    memcpy(string->chars + x->length, y->chars, y->length);
    return string;
}
