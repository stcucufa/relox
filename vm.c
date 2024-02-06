#ifdef DEBUG
#include <stdio.h>
#endif

#include "vm.h"

void chunk_init(Chunk* chunk) {
    byte_array_init(&chunk->bytes);
    number_array_init(&chunk->line_numbers);
    value_array_init(&chunk->values);
}

void chunk_add_byte(Chunk* chunk, uint8_t byte, size_t line_number) {
    byte_array_push(&chunk->bytes, byte);
    size_t n = chunk->line_numbers.count;
    if (n == 0 || chunk->line_numbers.items[n - 2] != line_number) {
        number_array_push(&chunk->line_numbers, line_number);
        number_array_push(&chunk->line_numbers, 1);
    } else {
        chunk->line_numbers.items[n - 1] += 1;
    }
}

size_t chunk_add_constant(Chunk* chunk, Value v) {
    // TODO too many constants (more than UINT8_MAX)
    value_array_push(&chunk->values, v);
    return chunk->values.count - 1;
}

void chunk_free(Chunk* chunk) {
    byte_array_free(&chunk->bytes);
    number_array_free(&chunk->line_numbers);
    value_array_free(&chunk->values);
    chunk_init(chunk);
}

#ifdef DEBUG

char const*const opcodes[opcode_count] = {
    [op_zero] = "zero",
    [op_one] = "one",
    [op_constant] = "constant",
    [op_negate] = "negate",
    [op_add] = "add",
    [op_subtract] = "subtract",
    [op_multiply] = "multiply",
    [op_divide] = "divide",
    [op_return] = "return",
};

void chunk_debug(Chunk* chunk, const char* name) {
    fprintf(stderr, "*** ----8<---- %s ----8<----\n", name);
    for (size_t i = 0, j = 0, k = 0; i < chunk->bytes.count;) {
        uint8_t opcode = chunk->bytes.bytes[i];
        size_t line = chunk->line_numbers.items[j];
        fprintf(stderr, "*** %4zu %4zu  %02x ", i, line, opcode);
        i += 1;
        k += 1;
        switch (opcode) {
            case op_zero:
            case op_one:
            case op_negate:
            case op_add:
            case op_subtract:
            case op_multiply:
            case op_divide:
            case op_return:
                fprintf(stderr, "    %s\n", opcodes[opcode]);
                break;
            case op_constant: {
                uint8_t arg = chunk->bytes.bytes[i];
                fprintf(stderr, "%02x  %s %g\n", arg, opcodes[opcode], chunk->values.items[arg]);
                i += 1;
                k += 1;
                break;
             }
            default:
                fprintf(stderr, "???\n");
        }
        if (k == chunk->line_numbers.items[j + 1]) {
            j += 2;
            k = 0;
        }
    }
    fprintf(stderr, "*** ----8<---- %s ----8<----\n", name);
}

#endif

result vm_run(VM* vm, Chunk* chunk) {
    vm->chunk = chunk;
    vm->ip = chunk->bytes.bytes;
    vm->sp = vm->stack;

#define BYTE() *vm->ip++
#define PUSH(x) *vm->sp++ = (x)
#define POP() *(--vm->sp)
#define PEEK(i) *(vm->sp - 1 - i)
#define POKE(i, x) *(vm->sp - 1 - i) = (x)
#define BINARY_OP(op) do { \
    double v = POP(); \
    POKE(0, PEEK(0) op v); \
} while (0)

    uint8_t opcode;
    do {
#ifdef DEBUG
        fprintf(stderr, "~~~ %4zu ", vm->ip - chunk->bytes.bytes);
#endif
        switch (opcode = BYTE()) {
            case op_zero: PUSH(0); break;
            case op_one: PUSH(1); break;
            case op_constant: PUSH(chunk->values.items[BYTE()]); break;
            case op_negate: POKE(0, -PEEK(0)); break;
            case op_add: BINARY_OP(+); break;
            case op_subtract: BINARY_OP(-); break;
            case op_multiply: BINARY_OP(*); break;
            case op_divide: BINARY_OP(/); break;
        }
#ifdef DEBUG
        fprintf(stderr, "%s {", opcodes[opcode]);
        for (Value* sp = vm->stack; sp != vm->sp; ++sp) {
            fprintf(stderr, " %g", *sp);
        }
        fprintf(stderr, " }\n");
#endif
    } while (opcode != op_return);

    return result_ok;

#undef BYTE
#undef PUSH
#undef POP
#undef POKE
#undef PEEK
#undef BINARY_OP
}
