#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "object.h"

typedef union {
    double as_double;
    uint64_t as_int;
} Value;

#define QNAN 0x7ffc000000000000
#define OBJECT_MASK 0x0000fffffffffff8

enum {
    tag_nan = 0,
    tag_nil = 1,
    tag_false = 2,
    tag_true = 3,
    tag_string = 4,
    tag_mask = 7,
};

#define VALUE_NIL (Value){ .as_int = QNAN | tag_nil }
#define VALUE_FALSE (Value){ .as_int = QNAN | tag_false }
#define VALUE_TRUE (Value){ .as_int = QNAN | tag_true }
#define VALUE_FROM_STRING(x) (Value){ .as_int = (uintptr_t)(x) | QNAN | tag_string }
#define VALUE_FROM_NUMBER(x) (Value){ .as_double = (x) }

#define VALUE_TAG(v) ((v).as_int & tag_mask)

#define VALUE_IS_NIL(v) (VALUE_TAG(v) == tag_nil)
#define VALUE_IS_BOOLEAN(v) (((v).as_int & 6) == 2)
#define VALUE_IS_FALSE(v) (VALUE_TAG(v) == tag_false)
#define VALUE_IS_TRUE(v) (VALUE_TAG(v) == tag_true)
#define VALUE_IS_STRING(v) (VALUE_TAG(v) == tag_string)
#define VALUE_IS_NUMBER(v) (((v).as_int & QNAN) != QNAN)

#define VALUE_TO_STRING(v) ((String*)((v).as_int & OBJECT_MASK))
#define VALUE_TO_CSTRING(v) (VALUE_TO_STRING(v)->chars)

void value_print(FILE*, Value);
bool value_equal(Value, Value);
void value_free_object(Value);

#endif
