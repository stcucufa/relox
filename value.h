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
    tag_pointer = 5,
    tag_count = 6,
    tag_mask = 7,
};

#define VALUE_QNAN_MASK 0x7ffc000000000000
#define VALUE_NONE_MASK (VALUE_QNAN_MASK | 0x0001000000000000)
#define VALUE_HAMT_NODE_MASK (VALUE_QNAN_MASK | 0x0002000000000000)
#define VALUE_EPSILON_MASK (VALUE_QNAN_MASK | tag_string)
#define VALUE_OBJECT_MASK 0x0000fffffffffff8
#define VALUE_SHORT_STRING_MASK 0x8000000000000004
#define VALUE_SHORT_STRING_LENGTH_MASK 0x38

#define VALUE_EQUAL(v, w) (v.as_int == w.as_int)

#define VALUE_NIL (Value){ .as_int = VALUE_QNAN_MASK | tag_nil }
#define VALUE_FALSE (Value){ .as_int = VALUE_QNAN_MASK | tag_false }
#define VALUE_TRUE (Value){ .as_int = VALUE_QNAN_MASK | tag_true }
#define VALUE_NONE (Value){ .as_int = VALUE_NONE_MASK }
#define VALUE_HAMT_NODE (Value){ .as_int = VALUE_HAMT_NODE_MASK }
#define VALUE_EPSILON (Value){ .as_int = VALUE_EPSILON_MASK }
#define VALUE_FROM_STRING(x) (Value){ .as_int = (uintptr_t)(x) | VALUE_QNAN_MASK | tag_string }
#define VALUE_FROM_POINTER(x) (Value){ .as_int = (uintptr_t)(x) | VALUE_QNAN_MASK | tag_pointer }
#define VALUE_FROM_NUMBER(x) (Value){ .as_double = (x) }
#define VALUE_FROM_INT(x) (Value){ .as_double = (double)(x) }

#define VALUE_TAG(v) ((v).as_int & tag_mask)

#define VALUE_IS_NONE(v) ((v).as_int == VALUE_NONE_MASK)
#define VALUE_IS_HAMT_NODE(v) (((v).as_int & VALUE_HAMT_NODE_MASK) == VALUE_HAMT_NODE_MASK)
#define VALUE_IS_NIL(v) (VALUE_TAG(v) == tag_nil)
#define VALUE_IS_BOOLEAN(v) (((v).as_int & 6) == 2)
#define VALUE_IS_FALSE(v) (VALUE_TAG(v) == tag_false)
#define VALUE_IS_TRUE(v) (VALUE_TAG(v) == tag_true)
#define VALUE_IS_EPSILON(v) ((v).as_int == VALUE_EPSILON_MASK)
#define VALUE_IS_SHORT_STRING(v) (((v).as_int & VALUE_SHORT_STRING_MASK) == VALUE_SHORT_STRING_MASK)
#define VALUE_IS_STRING(v) (VALUE_TAG(v) == tag_string)
#define VALUE_IS_POINTER(v) (VALUE_TAG(v) == tag_pointer)
#define VALUE_IS_NUMBER(v) (((v).as_int & VALUE_QNAN_MASK) != VALUE_QNAN_MASK)

#define VALUE_TO_STRING(v) ((String*)((v).as_int & VALUE_OBJECT_MASK))
#define VALUE_TO_CSTRING(v) (VALUE_TO_STRING(v)->chars)
#define VALUE_TO_HAMT(v) ((HAMT*)((v).as_int & VALUE_OBJECT_MASK))
#define VALUE_TO_POINTER(v) ((v).as_int & VALUE_OBJECT_MASK)
#define VALUE_TO_INT(v) ((int64_t)(v).as_double)
#define VALUE_TO_HAMT_NODE_BITMAP(v) ((uint32_t)(v).as_int)

#define VALUE_SHORT_STRING_LENGTH(v) ((size_t)(((v).as_int & VALUE_SHORT_STRING_LENGTH_MASK) >> 3))

void value_print(Value);
void value_print_debug(FILE*, Value, bool);
Value value_copy_string(const char*, size_t);
char* value_to_cstring(Value);
Value value_stringify(Value);
Value value_concatenate_strings(Value, Value);
Value value_string_exponent(Value, double);
uint32_t value_hash(Value);
void value_free_object(Value);

#endif
