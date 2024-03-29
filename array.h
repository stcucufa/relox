#ifndef __ARRAY_H__
#define __ARRAY_H__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "value.h"

#define ARRAY_MIN_CAPACITY 8
#define ARRAY_GROW_FACTOR 1.618

typedef struct {
    size_t count;
    size_t capacity;
    uint8_t* items;
} ByteArray;

void byte_array_init(ByteArray*);
void byte_array_push(ByteArray*, uint8_t);
void byte_array_free(ByteArray*);

typedef struct {
    size_t count;
    size_t capacity;
    size_t* items;
} NumberArray;

void number_array_init(NumberArray*);
void number_array_push(NumberArray*, size_t);
void number_array_free(NumberArray*);

typedef struct {
    size_t count;
    size_t capacity;
    Value* items;
} ValueArray;

void value_array_init(ValueArray*);
void value_array_push(ValueArray*, Value);
Value value_array_pop(ValueArray*);
void value_array_free(ValueArray*);

#endif
