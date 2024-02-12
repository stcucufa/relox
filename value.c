#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "value.h"

void value_print(Value v) {
    value_printf(stdout, v, false);
}

void value_printf(FILE* stream, Value v, bool debug) {
    if (VALUE_IS_NUMBER(v)) {
        if (v.as_double == INFINITY) {
            fputs("∞", stream);
        } else if (v.as_double == -INFINITY) {
            fputs("-∞", stream);
        } else {
            fprintf(stream, "%g", v.as_double);
        }
    } else {
        switch (VALUE_TAG(v)) {
            case tag_nil: fprintf(stream, "nil"); break;
            case tag_false: fprintf(stream, "false"); break;
            case tag_true: fprintf(stream, "true"); break;
            case tag_string:
                if (debug) {
                    fputs(VALUE_IS_EPSILON(v) ? "ε" : VALUE_TO_CSTRING(v), stream);
                } else if (!VALUE_IS_EPSILON(v)) {
                    fputs(VALUE_TO_CSTRING(v), stream);
                }
                break;
            default: fprintf(stream, "???");
        }
    }

#ifdef DEBUG
    fprintf(stream, " <0x%" PRIx64 ">", v.as_int);
#endif
}

char const*const strings[tag_count] = {
    [tag_nan] = "nan",
    [tag_nil] = "nil",
    [tag_false] = "false",
    [tag_true] = "true",
};

Value value_stringify(Value v) {
    if (VALUE_IS_NUMBER(v)) {
        return VALUE_FROM_STRING(string_from_number(v.as_double));
    }
    size_t tag = VALUE_TAG(v);
    if (tag == tag_string) {
        return v;
    }
    return VALUE_FROM_STRING(string_copy(strings[tag], strlen(strings[tag])));
}

Value value_concatenate_strings(Value x, Value y) {
    return VALUE_IS_EPSILON(x) ? y : (VALUE_IS_EPSILON(y) ? x :
        VALUE_FROM_STRING(string_concatenate(VALUE_TO_STRING(x), VALUE_TO_STRING(y))));
}

Value value_string_exponent(Value base, double x) {
    size_t y = (size_t)round(x > 0 ? x : 0);
    return VALUE_IS_EPSILON(base) ? base :
        (y == 0 ? VALUE_EPSILON : VALUE_FROM_STRING(string_exponent(VALUE_TO_STRING(base), y)));
}

uint32_t value_hash(Value v) {
    return VALUE_IS_STRING(v) && !VALUE_IS_EPSILON(v) ? VALUE_TO_STRING(v)->hash :
        bytes_hash(v.as_bytes, 8);
}

void value_free_object(Value v) {
    if (VALUE_IS_STRING(v) && !VALUE_IS_EPSILON(v)) {
#ifdef DEBUG
        fprintf(stderr, "--- value_free_object() string \"%s\"\n", VALUE_TO_CSTRING(v));
#endif
        free(VALUE_TO_STRING(v));
    }
}
