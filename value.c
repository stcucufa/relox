#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "value.h"

void value_print(FILE* stream, Value v) {
    if (VALUE_IS_NUMBER(v)) {
        fprintf(stream, "%g", v.as_double);
    } else {
        switch (v.as_int & 7) {
            case 1: fprintf(stream, "nil"); break;
            case 2: fprintf(stream, "false"); break;
            case 3: fprintf(stream, "true"); break;
            case 4: fprintf(stream, "%s", VALUE_TO_CSTRING(v)); break;
            default: fprintf(stream, "???");
        }
    }

#ifdef DEBUG
    fprintf(stream, "=0x%" PRIx64, v.as_int);
#endif
}

bool value_equal(Value x, Value y) {
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
