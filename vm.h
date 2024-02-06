#ifndef __VM_H__
#define __VM_H__

#include <stdint.h>
#include <stddef.h>

#include "array.h"
#include "value.h"

typedef enum {
    op_zero,
    op_one,
    op_constant,
    op_negate,
    op_add,
    op_subtract,
    op_multiply,
    op_divide,
    op_exponent,
    op_return,
    op_nop,
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

#define STACK_SIZE 256

typedef struct {
    Chunk* chunk;
    Value stack[STACK_SIZE];
    Value* sp;
    uint8_t* ip;
} VM;

typedef enum {
    result_ok,
    result_compile_error,
    result_runtime_error,
} Result;

Result vm_run(VM*, const char*);

#endif
