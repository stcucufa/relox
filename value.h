#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "object.h"

typedef union {
    double as_double;
    uint64_t as_int;
    char as_bytes[8];
} Value;

enum {
    tag_nan = 0,
    tag_nil = 1,
    tag_false = 2,
    tag_true = 3,
    tag_string = 4,
    tag_count = 5,
    tag_mask = 7,
};

#define QNAN 0x7ffc000000000000
#define NONE (QNAN | 0x0001000000000000)
#define HAMT_NODE_BITMAP (QNAN | 0x0002000000000000)
#define EPSILON (QNAN | tag_string)
#define OBJECT_MASK 0x0000fffffffffff8

#define VALUE_EQUAL(v, w) (v.as_int == w.as_int)

#define VALUE_NIL (Value){ .as_int = QNAN | tag_nil }
#define VALUE_FALSE (Value){ .as_int = QNAN | tag_false }
#define VALUE_TRUE (Value){ .as_int = QNAN | tag_true }
#define VALUE_NONE (Value){ .as_int = NONE }
#define VALUE_HAMT_NODE_BITMAP (Value){ .as_int = HAMT_NODE_BITMAP }
#define VALUE_EPSILON (Value){ .as_int = EPSILON }
#define VALUE_FROM_STRING(x) (Value){ .as_int = (uintptr_t)(x) | QNAN | tag_string }
#define VALUE_FROM_NUMBER(x) (Value){ .as_double = (x) }
#define VALUE_FROM_INT(x) (Value){ .as_double = (double)(x) }

#define VALUE_TAG(v) ((v).as_int & tag_mask)

#define VALUE_IS_NONE(v) ((v).as_int == NONE)
#define VALUE_IS_HAMT_NODE_BITMAP(v) (((v).as_int & HAMT_NODE_BITMAP) == HAMT_NODE_BITMAP)
#define VALUE_IS_NIL(v) (VALUE_TAG(v) == tag_nil)
#define VALUE_IS_BOOLEAN(v) (((v).as_int & 6) == 2)
#define VALUE_IS_FALSE(v) (VALUE_TAG(v) == tag_false)
#define VALUE_IS_TRUE(v) (VALUE_TAG(v) == tag_true)
#define VALUE_IS_EPSILON(v) ((v).as_int == EPSILON)
#define VALUE_IS_STRING(v) (VALUE_TAG(v) == tag_string)
#define VALUE_IS_NUMBER(v) (((v).as_int & QNAN) != QNAN)

#define VALUE_TO_STRING(v) ((String*)((v).as_int & OBJECT_MASK))
#define VALUE_TO_CSTRING(v) (VALUE_TO_STRING(v)->chars)
#define VALUE_TO_INT(v) ((int64_t)(v).as_double)
#define VALUE_TO_HAMT_NODE_BITMAP(v) ((uint32_t)(v).as_int)

void value_print(Value);
void value_printf(FILE*, Value, bool);
Value value_stringify(Value);
Value value_concatenate_strings(Value, Value);
Value value_string_exponent(Value, double);
uint64_t value_hash(Value);
void value_free_object(Value);

#endif
