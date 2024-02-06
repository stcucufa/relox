#pragma once

#include <stdint.h>
#include <stddef.h>

#include "array.h"
#include "value.h"

typedef enum {
    op_zero,
    op_one,
    op_constant,
    opcode_count
} Opcode;

typedef struct {
    ByteArray bytes;
    NumberArray line_numbers;
    ValueArray values;
} Chunk;

void chunk_init(Chunk*);
void chunk_add_byte(Chunk*, uint8_t, size_t);
size_t chunk_add_constant(Chunk*, Value);
void chunk_free(Chunk*);

#ifdef DEBUG
void chunk_debug(Chunk*, const char*);
#endif
