#pragma once

#include <stddef.h>
#include <stdint.h>

#define ARRAY_MIN_CAPACITY 8
#define ARRAY_GROW_FACTOR 1.6

typedef struct {
    size_t count;
    size_t capacity;
    uint8_t* bytes;
} ByteArray;

void byte_array_init(ByteArray*);
void byte_array_push(ByteArray*, uint8_t);
void byte_array_free(ByteArray*);
