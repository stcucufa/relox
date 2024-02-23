#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "value.h"

void value_print(Value v) {
    value_print_debug(stdout, v, false);
}

void value_print_debug(FILE* stream, Value v, bool debug) {
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
                if (VALUE_IS_SHORT_STRING(v)) {
                    size_t n = VALUE_SHORT_STRING_LENGTH(v);
                    for (size_t i = 0, offset = 6; i < n; ++i, offset += 7) {
                        fputc((v.as_int >> offset) & 0x7f, stream);
                    }
                } else if (debug) {
                    fputs(VALUE_IS_EPSILON(v) ? "ε" : VALUE_TO_CSTRING(v), stream);
                } else if (!VALUE_IS_EPSILON(v)) {
                    fputs(VALUE_TO_CSTRING(v), stream);
                }
                break;
            default: fprintf(stream, "???");
        }
    }
    if (debug) {
        fprintf(stream, " <0x%" PRIx64 ">", v.as_int);
    }
}

// Attempt to build a short string of six characters or less. Fails on
// characters that have the high bit set.
static Value value_short_string(const char* chars, size_t length) {
    Value str = (Value){ .as_int = VALUE_QNAN_MASK | VALUE_SHORT_STRING_MASK | (length << 3) };
    for (size_t i = 0, shift = 6; i < length; ++i, shift += 7) {
        if ((uint8_t)chars[i] > 0x7f) {
            return VALUE_NONE;
        }
        str.as_int |= ((uint64_t)chars[i] << shift);
    }
    return str;
}

Value value_copy_string(const char* start, size_t length) {
    if (length <= 6) {
        Value str = value_short_string(start, length);
        if (!VALUE_IS_NONE(str)) {
            return str;
        }
    }
    return VALUE_FROM_STRING(string_copy(start, length));
}

char const*const strings[tag_count] = {
    [tag_nan] = "nan",
    [tag_nil] = "nil",
    [tag_false] = "false",
    [tag_true] = "true",
};

Value value_stringify(Value v) {
    if (VALUE_IS_NUMBER(v)) {
        return VALUE_FROM_STRING(
            v.as_double == INFINITY ? string_copy("∞", 3) :
            v.as_double == -INFINITY ? string_copy("-∞", 4) :
            string_from_number(v.as_double)
        );
    }
    size_t tag = VALUE_TAG(v);
    if (tag == tag_string) {
        return v;
    }
    return VALUE_FROM_STRING(string_copy(strings[tag], strlen(strings[tag])));
}

char* value_to_cstring(Value v) {
    static char short_string[7];

    if (VALUE_IS_EPSILON(v)) {
        short_string[0] = 0;
        return &short_string[0];
    }

    if (VALUE_IS_SHORT_STRING(v)) {
        size_t n = VALUE_SHORT_STRING_LENGTH(v);
        for (size_t i = 0, shift = 6; i < n; ++i, shift += 7) {
            short_string[i] = (v.as_int >> shift) & 0x7f;
        }
        short_string[n] = 0;
        return &short_string[0];
    }

    return VALUE_TO_CSTRING(v);
}

static Value value_concatenate_short_short(Value x, Value y) {
    size_t m = VALUE_SHORT_STRING_LENGTH(x);
    size_t n = VALUE_SHORT_STRING_LENGTH(y);
    size_t length = m + n;
    String* string = string_new(length);
    for (size_t i = 0, shift = 6; i < m; ++i, shift += 7) {
        string->chars[i] = (x.as_int >> shift) & 0x7f;
    }
    for (size_t i = 0, shift = 6; i < n; ++i, shift += 7) {
        string->chars[i + m] = (y.as_int >> shift) & 0x7f;
    }
    string->chars[length] = 0;
    string->hash = bytes_hash(string->chars, string->length);
    return VALUE_FROM_STRING(string);
}

static Value value_concatenate_short_long(Value x, Value y) {
    size_t m = VALUE_SHORT_STRING_LENGTH(x);
    String* yy = VALUE_TO_STRING(y);
    size_t n = yy->length;
    size_t length = m + n;
    String* string = string_new(length);
    for (size_t i = 0, shift = 6; i < m; ++i, shift += 7) {
        string->chars[i] = (x.as_int >> shift) & 0x7f;
    }
    memcpy(string->chars + m, yy->chars, n);
    string->chars[length] = 0;
    string->hash = bytes_hash(string->chars, string->length);
    return VALUE_FROM_STRING(string);
}

static Value value_concatenate_long_short(Value x, Value y) {
    String* xx = VALUE_TO_STRING(x);
    size_t m = xx->length;
    size_t n = VALUE_SHORT_STRING_LENGTH(y);
    size_t length = m + n;
    String* string = string_new(length);
    memcpy(string->chars, xx->chars, m);
    for (size_t i = 0, shift = 6; i < n; ++i, shift += 7) {
        string->chars[i + m] = (y.as_int >> shift) & 0x7f;
    }
    string->chars[length] = 0;
    string->hash = bytes_hash(string->chars, string->length);
    return VALUE_FROM_STRING(string);
}

Value value_concatenate_strings(Value x, Value y) {
    if (VALUE_IS_EPSILON(y)) {
        // ε * y = y
        return x;
    }
    if (VALUE_IS_EPSILON(y)) {
        // x * ε = x
        return x;
    }
    if (VALUE_IS_SHORT_STRING(x)) {
        if (VALUE_IS_SHORT_STRING(y)) {
            size_t m = VALUE_SHORT_STRING_LENGTH(x);
            size_t n = VALUE_SHORT_STRING_LENGTH(y);
            size_t l = m + n;
            if (l <= 6) {
                // x * y is still a short string when |x| + |y| fits in a short string. Use the x string,
                // mask off the length bits, and OR with the y chars shifted by |x| and the new length.
                return (Value){ .as_int = (x.as_int & ~VALUE_SHORT_STRING_LENGTH_MASK) |
                    ((y.as_int & (((1 << (7 * n)) - 1) << 6)) << (7 * m)) | (l << 3) };
            }
            // x * y is too long to fit in a short string.
            return value_concatenate_short_short(x, y);
        }
        return value_concatenate_short_long(x, y);
    }
    if (VALUE_IS_SHORT_STRING(y)) {
        return value_concatenate_long_short(x, y);
    }
    return VALUE_FROM_STRING(string_concatenate(VALUE_TO_STRING(x), VALUE_TO_STRING(y)));
}

Value value_string_exponent(Value base, double x) {
    size_t y = (size_t)round(x > 0 ? x : 0);
    if (VALUE_IS_EPSILON(base) || y == 0) {
        // ε ** x = ε
        // base ** 0 = ε
        return VALUE_EPSILON;
    }
    if (y == 1) {
        // base ** 1 = base
        return base;
    }
    if (VALUE_IS_SHORT_STRING(base)) {
        size_t m = VALUE_SHORT_STRING_LENGTH(base);
        size_t n = m * y;
        if (n <= 6) {
            Value str = (Value){ .as_int = (base.as_int & ~VALUE_SHORT_STRING_LENGTH_MASK) | (n << 3) };
            uint64_t b = base.as_int & (((1 << (7 * m)) - 1) << 6);
            for (size_t i = 1; i < y; ++i) {
                str.as_int |= (b << (7 * m * i));
            }
            return str;
        }
        String* string = string_new(n);
        for (size_t i = 0; i < y; ++i) {
            for (size_t j = 0, shift = 6; j < m; ++j, shift += 7) {
                string->chars[i * m + j] = (base.as_int >> shift) & 0x7f;
            }
        }
        string->chars[n] = 0;
        string->hash = bytes_hash(string->chars, string->length);
        return VALUE_FROM_STRING(string);
    }
    return VALUE_FROM_STRING(string_exponent(VALUE_TO_STRING(base), y));
}

uint32_t value_hash(Value v) {
    return VALUE_IS_STRING(v) && !VALUE_IS_EPSILON(v) && !VALUE_IS_SHORT_STRING(v) ?
        VALUE_TO_STRING(v)->hash : bytes_hash(v.as_bytes, 8);
}

void value_free_object(Value v) {
    if (VALUE_IS_STRING(v) && !VALUE_IS_EPSILON(v) && !VALUE_IS_SHORT_STRING(v)) {
#ifdef DEBUG
        fprintf(stderr, "--- value_free_object() string \"%s\"\n", VALUE_TO_CSTRING(v));
#endif
        free(VALUE_TO_STRING(v));
    } else if (VALUE_IS_POINTER(v)) {
#ifdef DEBUG
        fprintf(stderr, "--- value_free_object() pointer %p\n", (void*)VALUE_TO_POINTER(v));
#endif
        free((void*)VALUE_TO_POINTER(v));
    }
}
