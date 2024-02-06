#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef union {
    double as_double;
    uint64_t as_int;
} Value;

#define QNAN 0x7ffc000000000000

#define VALUE_NIL (Value){ .as_int = QNAN | 1 }
#define VALUE_FALSE (Value){ .as_int = QNAN | 2 }
#define VALUE_TRUE (Value){ .as_int = QNAN | 3 }
#define VALUE_FROM_NUMBER(x) (Value){ .as_double = (x) }

#define VALUE_IS_NIL(x) (((x).as_int & (QNAN | 1)) == (QNAN | 1))
#define VALUE_IS_BOOLEAN(x) (((x).as_int & (QNAN | 6)) == (QNAN | 2))
#define VALUE_IS_FALSE(x) (((x).as_int & (QNAN | 2)) == (QNAN | 2))
#define VALUE_IS_TRUE(x) (((x).as_int & (QNAN | 3)) == (QNAN | 3))
#define VALUE_IS_NUMBER(x) (((x).as_int & QNAN) != QNAN)

void value_print(FILE*, Value);
bool value_equal(Value, Value);

#endif
