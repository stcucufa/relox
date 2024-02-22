#ifndef __VM_H__
#define __VM_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "array.h"
#include "hamt.h"
#include "object.h"
#include "value.h"

typedef enum {
    op_nil,
    op_zero,
    op_one,
    op_infinity,
    op_epsilon,
    op_constant,
    op_negate,
    op_add,
    op_subtract,
    op_multiply,
    op_divide,
    op_exponent,
    op_false,
    op_true,
    op_not,
    op_eq,
    op_ne,
    op_gt,
    op_ge,
    op_lt,
    op_le,
    op_bars,
    op_quote,
    op_print,
    op_pop,
    op_dup,
    op_define_global,
    op_get_global,
    op_set_global,
    op_get_local,
    op_set_local,
    op_jump,
    op_jump_true,
    op_jump_false,
    op_return,
    op_nop,
    opcode_count
} Opcode;

typedef struct VM VM;

typedef struct {
    struct VM* vm;
    ByteArray bytes;
    NumberArray line_numbers;
    ValueArray values;
} Chunk;

void chunk_init(Chunk*);
void chunk_add_byte(Chunk*, uint8_t, size_t);
uint8_t chunk_add_constant(Chunk*, HAMT*, Value);
void chunk_free(Chunk*);

#ifdef DEBUG
void chunk_debug(Chunk*, const char*);
#endif

#define STACK_SIZE 256

typedef struct {
    uint8_t index;
    bool initialized;
    bool mutable;
    bool global;
} Var;

typedef struct VM {
    Chunk* chunk;
    Value stack[STACK_SIZE];
    Value* sp;
    uint8_t* ip;
    HAMT global_scope;
    HAMT strings;
    ValueArray objects;
    ValueArray globals;
} VM;

typedef enum {
    result_ok,
    result_compile_error,
    result_runtime_error,
} Result;

Result vm_run(VM*, const char*);
Value vm_add_object(VM*, Value);
Var* vm_var_new(VM*, size_t, bool, bool);
Var* vm_add_global(VM*, Value, bool);
void vm_free(VM*);

#endif
