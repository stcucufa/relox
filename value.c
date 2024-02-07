#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "value.h"

void value_print(FILE* stream, Value v) {
    if (VALUE_IS_NUMBER(v)) {
        fprintf(stream, "%g", v.as_double);
    } else {
        switch (VALUE_TAG(v)) {
            case tag_nil: fprintf(stream, "nil"); break;
            case tag_false: fprintf(stream, "false"); break;
            case tag_true: fprintf(stream, "true"); break;
            case tag_string: fprintf(stream, "%s", VALUE_TO_CSTRING(v)); break;
            default: fprintf(stream, "???");
        }
    }

#ifdef DEBUG
    fprintf(stream, "=0x%" PRIx64, v.as_int);
#endif
}

bool value_equal(Value x, Value y) {
    if (VALUE_IS_STRING(x) && VALUE_IS_STRING(y)) {
        String *s = VALUE_TO_STRING(x);
        String *t = VALUE_TO_STRING(y);
        return s->length == t->length && memcmp(s->chars, t->chars, s->length) == 0;
    }
    return x.as_int == y.as_int;
}

Value value_string_copy(const char* start, size_t length) {
    String* string = malloc(sizeof(String));
    string->length = length;
    string->chars = malloc(length + 1);
    memcpy(string->chars, start, length);
    string->chars[length] = 0;
    return VALUE_FROM_STRING(string);
}

Value value_string_concatenate(Value x, Value y) {
    String* s = VALUE_TO_STRING(x);
    String* t = VALUE_TO_STRING(y);
    String* st = malloc(sizeof(String));
    st->length = s->length + t->length;
    st->chars = malloc(st->length + 1);
    memcpy(st->chars, s->chars, s->length);
    memcpy(st->chars + s->length, t->chars, t->length);
    return VALUE_FROM_STRING(st);
}
