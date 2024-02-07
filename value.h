#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    int length;
    char* chars;
} String;

typedef union {
    double as_double;
    uint64_t as_int;
} Value;

#define QNAN 0x7ffc000000000000
#define OBJECT_MASK 0x0000fffffffffff8

#define VALUE_NIL (Value){ .as_int = QNAN | 1 }
#define VALUE_FALSE (Value){ .as_int = QNAN | 2 }
#define VALUE_TRUE (Value){ .as_int = QNAN | 3 }
#define VALUE_FROM_STRING(x) (Value){ .as_int = (uintptr_t)(x) | QNAN | 4 }
#define VALUE_FROM_NUMBER(x) (Value){ .as_double = (x) }

#define VALUE_IS_NIL(v) (((v).as_int & (QNAN | 1)) == (QNAN | 1))
#define VALUE_IS_BOOLEAN(v) (((v).as_int & (QNAN | 6)) == (QNAN | 2))
#define VALUE_IS_FALSE(v) (((v).as_int & (QNAN | 2)) == (QNAN | 2))
#define VALUE_IS_TRUE(v) (((v).as_int & (QNAN | 3)) == (QNAN | 3))
#define VALUE_IS_STRING(v) (((v).as_int & QNAN) == (QNAN | 4))
#define VALUE_IS_NUMBER(v) (((v).as_int & QNAN) != QNAN)

#define VALUE_TO_CSTRING(v) (((String*)((v).as_int & OBJECT_MASK))->chars)

void value_print(FILE*, Value);
bool value_equal(Value, Value);
Value value_string_copy(const char*, size_t);

#endif
