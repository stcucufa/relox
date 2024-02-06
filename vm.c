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
};

void chunk_debug(Chunk* chunk, const char* name) {
    fprintf(stderr, "*** ----8<---- %s ----8<----\n", name);
    for (size_t i = 0, j = 0, k = 0; i < chunk->bytes.count;) {
        uint8_t opcode = chunk->bytes.bytes[i];
        size_t line = chunk->line_numbers.items[j];
        fprintf(stderr, "%4zu %4zu  %02x ", i, line, opcode);
        i += 1;
        k += 1;
        switch (opcode) {
            case op_zero:
            case op_one:
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
        if (k == chunk->line_numbers.items[j]) {
            j += 2;
            k = 0;
        }
    }
    fprintf(stderr, "*** ----8<---- %s ----8<----\n", name);
}

#endif
