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
